/****************************************************************************
 * apps/mini_cannode/cannode_main.c
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
 *   www.railab.me/posts/2025/2/nuttx-and-small-systems-can-node-example/
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>

#include <nuttx/leds/userled.h>
#include <nuttx/input/buttons.h>

#include "cannode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration */

#if (CONFIG_RAILAB_MINIMAL_CANNODE_NODEID & ~CANMSG_NODEID_MASK) != 0
#  error node ID must fit in CANMSG_NODEID_MASK mask
#endif

#if CONFIG_RAILAB_MINIMAL_CANNODE_HBSEC <= 0
#  error invalida value for CONFIG_RAILAB_MINIMAL_CANNODE_HBSEC
#endif

/* Devs path */

#define LEDS_DEVPATH   "/dev/userleds"
#define BUTTON_DEVPATH "/dev/buttons"

/* Threads parameters  */

#define TH_CANRX_PRIORTY    100
#define TH_BUTTON_PRIORTY   100

#define TH_CANRX_STACKSIZE  CONFIG_RAILAB_MINIMAL_CANNODE_RX_STACKSIZE
#define TH_BUTTON_STACKSIZE CONFIG_RAILAB_MINIMAL_CANNODE_TX_STACKSIZE

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_get_timestamp
 *
 * Description:
 *   Get timestamp.
 *
 ****************************************************************************/

static inline uint64_t can_get_timestamp(void)
{
  struct timespec ts;

  clock_systime_timespec(&ts);
  return 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;
}

#ifdef CONFIG_INPUT_BUTTONS
/****************************************************************************
 * Name: button_init
 *
 * Description:
 *   Initialzie Button interface.
 *
 ****************************************************************************/

int button_init(void)
{
  int fd;

  /* Open user button device */

  fd = open(BUTTON_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      PRINTF("failed to open %s %d\n", BUTTON_DEVPATH, -errno);
      return -errno;
    }

  return fd;
}

/****************************************************************************
 * Name: button_get
 *
 * Description:
 *   Get button state (blocking).
 *
 ****************************************************************************/

btn_buttonset_t button_get(int fd)
{
  btn_buttonset_t sample = 0;
  int             ret;

  ret = read(fd, (void *)&sample, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      PRINTF("read failed %d\n", -errno);
    }

  return sample;
}
#endif

#ifdef CONFIG_USERLED
/****************************************************************************
 * Name: led_get
 *
 * Description:
 *   Initialize LED interface.
 *
 ****************************************************************************/

int led_init(void)
{
  int fd;

  /* Open user LED device */

  fd = open(LEDS_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      PRINTF("failed to open %s %d\n", LEDS_DEVPATH, -errno);
      return -errno;
    }

  return fd;
}

/****************************************************************************
 * Name: led_get
 *
 * Description:
 *   Set LED state.
 *
 ****************************************************************************/

int led_set(int fd, bool state)
{
  userled_set_t ledset;
  int ret;

  /* Set LED */

  ledset = (state << 0);
  ret = ioctl(fd, ULEDIOC_SETALL, ledset);
  if (ret < 0)
    {
      PRINTF("ioctl failed %d\n", -errno);
      return -errno;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: heartbeat_msg
 *
 * Description:
 *   Fill heartbeat frame for CAN character device.
 *
 ****************************************************************************/

void heartbeat_msg(FAR struct cannode_env_s *env, FAR struct canmsg_s *msg,
                   uint64_t ts)
{
  msg->id  = env->nodeid + CANMSG_HEARTBEAT_ID;
  msg->len = CANMSG_HEARTBEAT_BYTES;

  /* Copy timestamp as data */

  memcpy(msg->data, &ts, sizeof(uint64_t));
}

#if defined(CONFIG_INPUT_BUTTONS) || defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
/****************************************************************************
 * Name: button_msg
 *
 * Description:
 *   Fill button message for CAN character device.
 *
 ****************************************************************************/

void button_msg(FAR struct cannode_env_s *env, FAR struct canmsg_s *msg,
                btn_buttonset_t sample)
{
  msg->id  = env->nodeid + CANMSG_BUTTON_ID;
  msg->len = CANMSG_BUTTON_BYTES;

  /* Send button state */

  msg->data[0] = sample;
  msg->data[1] = 0;
  msg->data[2] = 0;
  msg->data[3] = 0;
  msg->data[4] = 0;
  msg->data[5] = 0;
  msg->data[6] = 0;
  msg->data[7] = 0;
}
#endif

#if defined(CONFIG_USERLED) || defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
/****************************************************************************
 * Name: thread_rx
 *
 * Description:
 *   Wait for CAN frames and handle request.
 *
 ****************************************************************************/

FAR void *thread_rx(FAR void* data)
{
  FAR struct cannode_env_s *env = (FAR struct cannode_env_s *)data;
  struct canmsg_s           msg;
  uint32_t                  msgid;
  int                       ret;
#ifdef CONFIG_USERLED
  int                       fd;
#endif

  DEBUGASSERT(env);

#ifdef CONFIG_USERLED
  /* Initialzie LED */

  fd = led_init();
  if (fd < 0)
    {
      PRINTF("led_init failed %d\n", fd);
      return NULL;
    }
#endif

  while (1)
    {
      /* Wait for data */

      ret = can_read(env->fd, &msg);
      if (ret > 0)
        {
          /* Get only frames that are addressed to us */

          if ((msg.id & CANMSG_NODEID_MASK) != env->nodeid)
            {
              continue;
            }

          /* Get msgid */

          msgid = msg.id - env->nodeid;

          /* Handle messages */

          switch (msgid)
            {
              case CANMSG_LED_ID:
                {
#ifdef CONFIG_USERLED
                  /* Set LED */

                  ret = led_set(fd, msg.data[0]);
                  if (ret < 0)
                    {
                      PRINTF("led_set failed %d\n", ret);
                    }
#else
                  /* Just print message request */

                  printf("Set LED request %d\n", msg.data[0]);
#endif
                }

              /* Ignore all the rest */

              default:
                {
                  break;
                }
            }
        }
      else
        {
          PRINTF("can_read failed %d\n", ret);
        }
    }

  return NULL;
}
#endif

#ifdef CONFIG_INPUT_BUTTONS
/****************************************************************************
 * Name: thread_button
 *
 * Description:
 *   Wait for button and report state change.
 *
 ****************************************************************************/

FAR void *thread_button(FAR void* data)
{
  FAR struct cannode_env_s *env = (FAR struct cannode_env_s *)data;
  struct canmsg_s           msg;
  struct pollfd             fds[1];
  btn_buttonset_t           sample;
  int                       fd;
  int                       ret;

  DEBUGASSERT(env);

  /* Initialzie button */

  fd = button_init();
  if (fd < 0)
    {
      PRINTF("button_init failed %d\n", fd);
      return NULL;
    }

  /* Initialzie poll */

  memset(fds, 0, sizeof(fds));

  fds[0].fd      = fd;
  fds[0].events  = POLLIN;

  while (1)
    {
      /* Wait for button */

      ret = poll(fds, 1, -1);
      if (ret < 0)
        {
          PRINTF("poll failed %d\n", -errno);
        }

      if (fds[0].revents & POLLIN)
        {
          fds[0].revents = 0;

          /* Send change in button state */

          sample = button_get(fd);

          /* Fill frame */

          button_msg(env, &msg, sample);

          /* Send frame */

          ret = can_send(env->fd, &msg);
          if (ret < 0)
            {
              PRINTF("can_send failed %d\n", ret);
            }
        }
    }

  return NULL;
}

#elif defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
/****************************************************************************
 * Name: thread_button
 *
 * Description:
 *   Simulate button state change
 *
 ****************************************************************************/

FAR void *thread_button(FAR void* data)
{
  FAR struct cannode_env_s *env = (FAR struct cannode_env_s *)data;
  struct canmsg_s           msg;
  btn_buttonset_t           sample;
  btn_buttonset_t           sample_last = 0;
  int                       ret;

  DEBUGASSERT(env);

  /* Seed the random number generator */

  srand(time(NULL));

  while (1)
    {
      /* Sleep some random time */

      sleep((rand() % 10) + 1);

      /* Get button state */

      sample = rand() % 2;

      /* Next cycle if sample is the same */

      if (sample_last == sample)
        {
          continue;
        }

      sample_last = sample;

      /* Fill frame */

      button_msg(env, &msg, sample);

      /* Send frame */

      ret = can_send(env->fd, &msg);
      if (ret < 0)
        {
          PRINTF("can_send failed %d\n", ret);
        }
    }

  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cannode_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct cannode_env_s env;
  struct canmsg_s      msg;
  int                  ret;
#if defined(CONFIG_USERLED) || defined(CONFIG_INPUT_BUTTONS) || \
    defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
  struct sched_param   param;
  pthread_attr_t       attr;
#endif

  /* Initialzie env */

  env.nodeid = CONFIG_RAILAB_MINIMAL_CANNODE_NODEID;
  env.fd     = -1;

  /* Initialize CAN */

  env.fd = can_init();
  if (env.fd < 0)
    {
      PRINTF("can_init failed %d\n", env.fd);
      return -1;
    }

#if defined(CONFIG_USERLED) || defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
  pthread_t th1;

  /* Create CAN rx thread */

  pthread_attr_init(&attr);
  param.sched_priority = TH_CANRX_PRIORTY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, TH_CANRX_STACKSIZE);

  ret = pthread_create(&th1, &attr, thread_rx, &env);
  if (ret != 0)
    {
      PRINTF("pthread_crete failed %d\n", ret);
      return -1;
    }
#endif

#if defined(CONFIG_INPUT_BUTTONS) || defined(CONFIG_RAILAB_MINIMAL_CANNODE_DUMMY)
  pthread_t th2;

  /* Create button thread */

  pthread_attr_init(&attr);
  param.sched_priority = TH_BUTTON_PRIORTY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, TH_BUTTON_STACKSIZE);

  ret = pthread_create(&th2, &attr, thread_button, &env);
  if (ret != 0)
    {
      PRINTF("pthread_crete failed %d\n", ret);
      return -1;
    }
#endif

  /* Main becomes heartbeat provider */

  while (1)
    {
      /* Fill heartbeat frame */

      heartbeat_msg(&env, &msg, can_get_timestamp());

      /* Send heartbeat frame */

      ret = can_send(env.fd, &msg);
      if (ret < 0)
        {
          PRINTF("can_send failed %d\n", ret);
          return -1;
        }

      /* Wait for the next interval */

      sleep(CONFIG_RAILAB_MINIMAL_CANNODE_HBSEC);
    }

  return 0;
}
