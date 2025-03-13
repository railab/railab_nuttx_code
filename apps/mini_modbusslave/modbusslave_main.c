/****************************************************************************
 * apps/mini_modbusslave/modbusslave_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* Details about this code can be found on:
 *   www.railab.me/posts/2025/3/nuttx-and-small-systems-modbus-slave-example/
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fixedmath.h>

#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#include "modbus/mb.h"
#include "modbus/mbport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Debug prints */

#if 0
#  define PRINTF(format, ...)   printf(format, ##__VA_ARGS__)
#else
#  define PRINTF(...)
#endif

/* Threads parameters  */

#define TH_ADC_PRIORTY           100
#define TH_ADC_STACKSIZE         512

#define TH_MODBUS_PRIORTY        100
#define TH_MODBUS_STACKSIZE      512

/* Modbus configuration */

#define MODBUS_PORT              0
#define MODBUS_BAUD              B19200
#define MODBUS_PARITY            2
#define MODBUS_ID                CONFIG_RAILAB_MINIMAL_MODBUSSLAVE_SLAVEID

/* ADC configuration */

#define ADC_DEVPATH              "/dev/adc0"
#define ADC_SAMPLES              4

/* ADC convertion factor */

#define ADC_RAW_MAX              4095
#define ADC_REF_MILIVOLT         3300
#define ADC_RAW_TO_MILIVOLT(raw) (((raw) * ADC_REF_MILIVOLT) / ADC_RAW_MAX)

/* Modbus input registers:
 *
 *   Reg 1  - status  (uint16_t)
 *   Reg 2  - counter (uint16_t)
 *   Reg 3  - ADC ch0 now (int16_t)
 *   Reg 4  - ADC ch1 now (int16_t)
 *   Reg 5  - ADC ch2 now (int16_t)
 *   Reg 6  - ADC ch3 now (int16_t)
 *   Reg 7  - ADC ch0 min (int16_t)
 *   Reg 8  - ADC ch1 min (int16_t)
 *   Reg 9  - ADC ch2 min (int16_t)
 *   Reg 10 - ADC ch3 min (int16_t)
 *   Reg 11 - ADC ch0 max (int16_t)
 *   Reg 12 - ADC ch1 max (int16_t)
 *   Reg 13 - ADC ch2 max (int16_t)
 *   Reg 14 - ADC ch3 max (int16_t)
 *   Reg 15 - ADC ch0 avg (int16_t)
 *   Reg 16 - ADC ch1 avg (int16_t)
 *   Reg 17 - ADC ch2 avg (int16_t)
 *   Reg 18 - ADC ch3 avg (int16_t)
 *
 * Modbus holding registers:
 *
 *   Reg 31 - ADC sampling intervali in us (uint16_t)
 *   Reg 32 - Reset ADC samples when written value greater than 0 (uint16_t)
 */

#define MODBUS_REG_INPUT_START    1
#define MODBUS_REG_INPUT_NREGS    18

#define MODBUS_REG_HOLDING_START  31
#define MODBUS_REG_HOLDING_NREGS  2

/* Device defaults */

#define INTERVAL_DEFAULT_US      10000
#define EMA_ALPHA_SHIFT          2        /* alpha = 1 / (2 ^ 1) = 0.5 */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Modbus input registers */

begin_packed_struct struct modbus_input_s
{
  uint16_t status;
  uint16_t cntr;
  int16_t  now[ADC_SAMPLES];    /* Sample now in mV */
  int16_t  min[ADC_SAMPLES];    /* Sample min in mV */
  int16_t  max[ADC_SAMPLES];    /* Sample max in mV */
  int16_t  avg[ADC_SAMPLES];    /* Sample exponential moving average in mV */
} end_packed_struct;

/* Modbus holding registers */

begin_packed_struct struct modbus_holding_s
{
  uint16_t interval;  /* Sampling interval */
  uint16_t rst_stats; /* Reset statistics */
} end_packed_struct;

/* Modbus state */

struct modbus_state_s
{
  struct modbus_input_s   input;    /* Input state */
  struct modbus_holding_s holding;  /* Holding state */
  pthread_mutex_t         lock;     /* Data lock */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct modbus_state_s g_modbus;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: device_initialize
 *
 * Description:
 *   Initialize device data
 *
 ****************************************************************************/

static int device_initialize(void)
{
  int ret;

  memset(&g_modbus, 0, sizeof(struct modbus_state_s));

  /* Initialize mutex */

  ret = pthread_mutex_init(&g_modbus.lock, NULL);
  if (ret != 0)
    {
      PRINTF("pthread_mutex_init failed %d\n", ret);
      return ret;
    }

  /* Initialize device data */

  g_modbus.holding.interval = INTERVAL_DEFAULT_US;
  g_modbus.input.status     = 0;

  return OK;
}

#ifdef CONFIG_MODBUS
/****************************************************************************
 * Name: modbus_initialize
 *
 * Description:
 *   Initialize FreeModBus stack
 *
 ****************************************************************************/

static int modbus_initialize(void)
{
  eMBErrorCode mberr;

  /* Initialize the FreeModBus library */

  mberr = eMBInit(MB_RTU, MODBUS_ID, MODBUS_PORT, MODBUS_BAUD,
                  MODBUS_PARITY);
  if (mberr != MB_ENOERR)
    {
      PRINTF("eMBInit failed: %d\n", mberr);
      return -1;
    }

  /* Enable FreeModBus */

  mberr = eMBEnable();
  if (mberr != MB_ENOERR)
    {
      PRINTF("eMBEnable failed: %d\n", mberr);
      return -1;
    }

  return OK;
}
#endif

#ifdef CONFIG_MODBUS
/****************************************************************************
 * Name: thread_modbus
 *
 * Description:
 *   Handle Modbus
 *
 ****************************************************************************/

static FAR void *thread_modbus(FAR void *data)
{
  eMBErrorCode mberr;
  int          ret;

  /* Initialize the modbus stack */

  ret = modbus_initialize();
  if (ret != OK)
    {
      PRINTF("modbus_initialize failed: %d\n", ret);
      return NULL;
    }

  while (true)
    {
      /* Poll modbus */

      mberr = eMBPoll();
      if (mberr != MB_ENOERR)
        {
          PRINTF("eMBPoll failed: %d\n", ret);
        }
    }

  return NULL;
}
#endif

#ifdef CONFIG_ADC
/****************************************************************************
 * Name: adc_reset_stats
 *
 * Description:
 *   Reset ADC stats.
 *
 ****************************************************************************/

static void adc_reset_stats(void)
{
  int i;

  for (i = 0; i < ADC_SAMPLES; i++)
    {
      g_modbus.input.min[i] = INT16_MAX;
      g_modbus.input.max[i] = INT16_MIN;
      g_modbus.input.avg[i] = 0;
    }
}

/****************************************************************************
 * Name: thread_adc
 *
 * Description:
 *   Wait for ADC data and handle samples
 *
 ****************************************************************************/

static FAR void *thread_adc(FAR void *data)
{
  struct adc_msg_s sample[ADC_SAMPLES];
  uint16_t         interval;
  int              ret;
  int              fd;
  int              i;

  /* Open ADC in blocking mode */

  fd = open(ADC_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      PRINTF("failed to open %s %d\n", ADC_DEVPATH, errno);
      return NULL;
    }

  pthread_mutex_lock(&g_modbus.lock);

  /* Reset ADC stats */

  adc_reset_stats();

  /* Device is ready now */

  g_modbus.input.status = 1;

  pthread_mutex_unlock(&g_modbus.lock);

  /* Handle samples */

  while (1)
    {
      /* Issue the software trigger to start ADC conversion */

      ret = ioctl(fd, ANIOC_TRIGGER, 0);
      if (ret < 0)
        {
          PRINTF("ANIOC_TRIGGER ioctl failed: %d\n", errno);
        }

      /* Get samples */

      ret = read(fd, sample, sizeof(struct adc_msg_s) * ADC_SAMPLES);
      if (ret < 0)
        {
          PRINTF("read failed: %d\n", errno);
        }

      /* Lock device data */

      pthread_mutex_lock(&g_modbus.lock);

      /* Check stats reset request */

      if (g_modbus.holding.rst_stats)
        {
          adc_reset_stats();
          g_modbus.holding.rst_stats = 0;
        }

      /* Store samples */

      for (i = 0; i < ADC_SAMPLES; i++)
        {
          g_modbus.input.now[i] = ADC_RAW_TO_MILIVOLT(sample[i].am_data);
          g_modbus.input.min[i] = MIN(g_modbus.input.now[i],
                                      g_modbus.input.min[i]);
          g_modbus.input.max[i] = MAX(g_modbus.input.now[i],
                                      g_modbus.input.max[i]);

          /* Exponential moving average where:
           * alpha = 1 / (2 ^ EMA_ALPHA_SHIFT)
           */

          g_modbus.input.avg[i] = (g_modbus.input.avg[i] +
                                   ((g_modbus.input.now[i] -
                                     g_modbus.input.avg[i]) >>
                                    EMA_ALPHA_SHIFT));
        }

      interval = g_modbus.holding.interval;

      /* Unlock device data */

      pthread_mutex_unlock(&g_modbus.lock);

      /* Wait for the next interval */

      usleep(interval);
    }

  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modbusslave_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct sched_param param;
  pthread_attr_t     attr;
  int                ret;

  /* Initialzie modbus state */

  ret = device_initialize();
  if (ret != 0)
    {
      PRINTF("device_initialize failed %d\n", ret);
      return -1;
    }

#ifdef CONFIG_MODBUS
  /* Start Modbus thread */

  pthread_t th1;

  pthread_attr_init(&attr);
  param.sched_priority = TH_MODBUS_PRIORTY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, TH_ADC_STACKSIZE);

  ret = pthread_create(&th1, &attr, thread_modbus, NULL);
  if (ret != 0)
    {
      PRINTF("pthread_crete failed %d\n", ret);
      return -1;
    }
#endif

#ifdef CONFIG_ADC
  /* Start ADC thread */

  pthread_t th2;

  /* Create ADC thread */

  pthread_attr_init(&attr);
  param.sched_priority = TH_ADC_PRIORTY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, TH_ADC_STACKSIZE);

  ret = pthread_create(&th2, &attr, thread_adc, NULL);
  if (ret != 0)
    {
      PRINTF("pthread_crete failed %d\n", ret);
      return -1;
    }
#endif

  /* Do some dummy periodic work in main */

  while (1)
    {
      /* Increase counter */

      pthread_mutex_lock(&g_modbus.lock);
      g_modbus.input.cntr += 1;
      pthread_mutex_unlock(&g_modbus.lock);

      sleep(1);
    }
}

/****************************************************************************
 * Name: eMBRegInputCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegInputCB(FAR uint8_t *buffer, uint16_t address,
                           uint16_t nregs)
{
  eMBErrorCode  mberr = MB_ENOERR;
  uint16_t     *ptr   = (uint16_t *)&g_modbus.input;
  int           index;

  /* Lock device data */

  pthread_mutex_lock(&g_modbus.lock);

  if ((address >= MODBUS_REG_INPUT_START)
      && (address + nregs
          <= MODBUS_REG_INPUT_START + MODBUS_REG_INPUT_NREGS))
    {
      index = (int)(address - MODBUS_REG_INPUT_START);

      while (nregs > 0)
        {
          *buffer++ = (uint8_t)(htobe16(ptr[index]) & 0xff);
          *buffer++ = (uint8_t)(htobe16(ptr[index]) >> 8);
          index++;
          nregs--;
        }
    }
  else
    {
      mberr = MB_ENOREG;
    }

  /* Unlock device data */

  pthread_mutex_unlock(&g_modbus.lock);

  return mberr;
}

/****************************************************************************
 * Name: eMBRegHoldingCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegHoldingCB(FAR uint8_t *buffer, uint16_t address,
                             uint16_t nregs, eMBRegisterMode mode)
{
  eMBErrorCode  mberr = MB_ENOERR;
  uint16_t     *ptr   = (uint16_t *)&g_modbus.holding;
  int           index;

  /* Lock device data */

  pthread_mutex_lock(&g_modbus.lock);

  if ((address >= MODBUS_REG_HOLDING_START)
      && (address + nregs
          <= MODBUS_REG_HOLDING_START + MODBUS_REG_HOLDING_NREGS))
    {
      index = (int)(address - MODBUS_REG_HOLDING_START);

      switch (mode)
        {
          case MB_REG_READ:
            {
              while (nregs > 0)
                {
                  *buffer++ = (uint8_t)(htobe16(ptr[index]) & 0xff);
                  *buffer++ = (uint8_t)(htobe16(ptr[index]) >> 8);
                  index++;
                  nregs--;
                }
              break;
            }

          case MB_REG_WRITE:
            {
              while (nregs > 0)
                {
                  ptr[index] = be16toh(*buffer++);
                  ptr[index] |= be16toh(*buffer++ << 8);
                  index++;
                  nregs--;
                }
              break;
            }
        }
    }
  else
    {
      mberr = MB_ENOREG;
    }

  /* Unlock device data */

  pthread_mutex_unlock(&g_modbus.lock);

  return mberr;
}

/****************************************************************************
 * Name: eMBRegCoilsCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegCoilsCB(FAR uint8_t *buffer, uint16_t address,
                           uint16_t ncoils, eMBRegisterMode mode)
{
  return MB_ENOREG;
}

/****************************************************************************
 * Name: eMBRegDiscreteCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegDiscreteCB(FAR uint8_t *buffer, uint16_t address,
                              uint16_t ndiscrete)
{
  return MB_ENOREG;
}
