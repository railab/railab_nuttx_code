/****************************************************************************
 * apps/mini_blinky_posix/blinky1_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include <nuttx/leds/userled.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LEDS_DEVPATH "/dev/userleds"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: blinky1_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  userled_set_t ledset;
  int ret;
  int fd;

  /* Open user LED device */

  fd = open(LEDS_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      return -1;
    }

  while (1)
    {
      /* Toggle LED */

      ledset ^= 1;

      /* Set LED */

      ret = ioctl(fd, ULEDIOC_SETALL, ledset);
      if (ret != 0)
        {
          return -1;
        }

      /* Wait some time */

      sleep(1);
    }

  return 0;
}
