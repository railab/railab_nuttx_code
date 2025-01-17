/****************************************************************************
 * apps/mini_blinky_stm32/blinky2_main.c
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
 *   www.railab.me/posts/2024/12/nuttx-and-small-systems-core-os/
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <unistd.h>

#include "stm32.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GPIO_LED1      (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz| \
                        GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN13)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: blinky2_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  bool ledon = false;

  /* Initialize LED GPIO */

  stm32_configgpio(GPIO_LED1);

  while (1)
    {
      /* Toggle LED */

      ledon ^= 1;

      /* Write LED */

      stm32_gpiowrite(GPIO_LED1, ledon);

      /* Wait some time */

      sleep(1);
    }

  return 0;
}
