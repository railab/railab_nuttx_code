/****************************************************************************
 * apps/mini_components4/componets3_main.c
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

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS4_THREAD
static void *thread(FAR void *data)
{
  while (1)
    {
      sleep(2);
      sem_post(&g_sem);
    }

  return NULL;
}
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS4_POSIXTIMER_SIGENV
static void sigev_callback(union sigval value)
{
  sem_post(&g_sem);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: components4_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("components4 examples\n");

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS4_THREAD
  pthread_t th;
  pthread_create(&th, NULL, thread, &sync);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS4_POSIXTIMER
  struct sigevent   notify;
  struct itimerspec timer;
  timer_t           timerid;

  notify.sigev_notify            = SIGEV_SIGNAL;
  notify.sigev_signo             = SIGRTMIN;
  notify.sigev_value.sival_int   = 0;
#  ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS4_POSIXTIMER_SIGENV
  notify.sigev_notify            = SIGEV_THREAD;
  notify.sigev_notify_function   = sigev_callback;
  notify.sigev_notify_attributes = NULL;
#  endif

  timer_create(CLOCK_REALTIME, &notify, &timerid);

  /* Set timer */

  timer.it_value.tv_sec     = 2;
  timer.it_value.tv_nsec    = 0;
  timer.it_interval.tv_sec  = 2;
  timer.it_interval.tv_nsec = 0;

  timer_settime(timerid, 0, &timer, NULL);
#endif

  while (1)
    {
      ret = sem_wait(&g_sem);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              printf("EINTR\n");
            }
        }
      else
        {
          printf("sem!\n");
        }
    }

  return 0;
}
