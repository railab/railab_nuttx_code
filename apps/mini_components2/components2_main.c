/****************************************************************************
 * apps/mini_components2/components2_main.c
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
 *   www.railab.me/posts/2025/01/nuttx-and-small-systems-os-components/
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdio.h>
#include <poll.h>
#include <sys/epoll.h>

#include <sys/timerfd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXFDS 2

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS2_TIMERFD
static int timer_read(int fd)
{
  timerfd_t tdret;
  int ret;

  ret = read(fd, &tdret, sizeof(timerfd_t));
  if (ret > 0)
    {
      printf("fd=%d\n", fd);
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: components2_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("components2 examples\n");

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS2_TIMERFD
  struct itimerspec tms;
  int               tfd1;
  int               tfd2;
  int               ret;

  UNUSED(ret);

  /* Create timers */

  tfd1 = timerfd_create(CLOCK_MONOTONIC, 0);
  tfd2 = timerfd_create(CLOCK_MONOTONIC, 0);

  /* Start timers */

  tms.it_value.tv_sec     = 2;
  tms.it_value.tv_nsec    = 0;
  tms.it_interval.tv_sec  = 2;
  tms.it_interval.tv_nsec = 0;

  timerfd_settime(tfd1, 0, &tms, NULL);
  timerfd_settime(tfd2, 0, &tms, NULL);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS2_POLL
  struct pollfd fds[MAXFDS];

  memset(fds, 0, sizeof(struct pollfd)*MAXFDS);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS2_EPOLL
  struct epoll_event ev;
  struct epoll_event events[MAXFDS];
  int                epollfd;

  epollfd = epoll_create1(EPOLL_CLOEXEC);

  /* Add timers to epoll */

  ev.events = EPOLLIN;
  ev.data.fd = tfd1;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd1, &ev);

  ev.events = EPOLLIN;
  ev.data.fd = tfd2;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd2, &ev);
#endif

  while (1)
    {
#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS2_POLL
      /* Wait with poll */

      fds[0].fd      = tfd1;
      fds[0].events  = POLLIN;
      fds[0].revents = 0;

      fds[1].fd      = tfd2;
      fds[1].events  = POLLIN;
      fds[1].revents = 0;

      ret = poll(fds, MAXFDS, -1);
      if (ret > 0)
        {
          if (fds[0].revents == POLLIN)
            {
              timer_read(tfd1);
            }

          if (fds[1].revents == POLLIN)
            {
              timer_read(tfd2);
            }
        }
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS2_EPOLL)
      /* Wait with epoll */

      ret = epoll_wait(epollfd, events, MAXFDS, -1);
      if (ret > 0)
        {
          if (events[0].events == POLLIN)
            {
              timer_read(events[0].data.fd);
            }

          if (events[1].events == POLLIN)
            {
              timer_read(events[1].data.fd);
            }
        }
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS2_TIMERFD)
      /* Busy wait for both timers */

      timer_read(tfd1);
      timer_read(tfd2);
#endif

      sleep(1);
    }

  return 0;
}
