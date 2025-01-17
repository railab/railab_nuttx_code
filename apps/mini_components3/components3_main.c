/****************************************************************************
 * apps/mini_components3/componets2_main.c
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
#include <pthread.h>
#include <semaphore.h>
#include <nuttx/atomic.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define THREADS 3

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_RAILAB_MINIMAL_COMPONENTS3_ATOMIC
static uint32_t g_val = 0;
#else
static atomic_uint g_val;
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_MUTEX
static pthread_mutex_t g_mut;
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_COND
static pthread_mutex_t g_mut;
static pthread_cond_t  g_cond;
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_RWLOCK
static pthread_rwlock_t g_rw;
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_THREADS
static pthread_t g_th[THREADS];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_MUTEX)
static void *thread(FAR void *data)
{
  int id  = (int)((intptr_t)data);

  printf("hello from %d\n", id);

  /* Protec with mutex */

  pthread_mutex_lock(&g_mut);
  g_val++;
  pthread_mutex_unlock(&g_mut);

  return NULL;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_ATOMIC)
static void *thread(FAR void *data)
{
  int id  = (int)((intptr_t)data);

  printf("hello from %d\n", id);

  /* Use atomic */

  atomic_fetch_add(&g_val, 1);

  return NULL;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_COND)
static void *thread(FAR void *data)
{
  int id  = (int)((intptr_t)data);

  printf("hello from %d\n", id);

  /* Use cond variable */

  pthread_mutex_lock(&g_mut);
  g_val++;
  pthread_cond_signal(&g_cond);
  pthread_mutex_unlock(&g_mut);

  return NULL;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_RWLOCK)
static void *thread(FAR void *data)
{
  int id  = (int)((intptr_t)data);

  printf("hello from %d\n", id);

  /* Use rwlock */

  pthread_rwlock_wrlock(&g_rw);
  g_val++;
  pthread_rwlock_unlock(&g_rw);

  return NULL;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_THREADS)
static void *thread(FAR void *data)
{
  int id  = (int)((intptr_t)data);

  printf("hello from %d\n", id);

  return NULL;
}
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_THREADS
static void threads_init(void)
{
  pthread_attr_t attr;

  for (int i = 0; i < THREADS; i++)
    {
      pthread_attr_init(&attr);
      attr.priority = PTHREAD_DEFAULT_PRIORITY - i;
      pthread_create(&g_th[i], &attr, thread, (pthread_addr_t)i);
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: components3_main
 ****************************************************************************/

#if defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_ATOMIC)
int main(int argc, FAR char *argv[])
{
  printf("components3 examples\n");

  atomic_init(&g_val, 0);

  threads_init();

  while (atomic_load(&g_val) < 3)
    {
      sleep(1);
    }

  printf("done!\n");

  return 0;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_MUTEX)
int main(int argc, FAR char *argv[])
{
  printf("components3 examples\n");

  uint32_t tmp;

  pthread_mutex_init(&g_mut, NULL);

  threads_init();

  do
    {
      sleep(1);

      pthread_mutex_lock(&g_mut);
      tmp = g_val;
      pthread_mutex_unlock(&g_mut);
    } while (tmp < 3);

  printf("done!\n");

  return 0;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_COND)
int main(int argc, FAR char *argv[])
{
  printf("components3 examples\n");

  pthread_cond_init(&g_cond, NULL);
  pthread_mutex_init(&g_mut, NULL);

  threads_init();

  pthread_mutex_lock(&g_mut);

  while (g_val < 3)
    {
      pthread_cond_wait(&g_cond, &g_mut);
    }

  pthread_mutex_unlock(&g_mut);

  printf("done!\n");

  return 0;
}
#elif defined(CONFIG_RAILAB_MINIMAL_COMPONENTS3_RWLOCK)
int main(int argc, FAR char *argv[])
{
  printf("components3 examples\n");

  uint32_t tmp;

  pthread_rwlock_init(&g_rw, NULL);

  threads_init();

  do
    {
      sleep(1);

      pthread_rwlock_rdlock(&g_rw);
      tmp = g_val;
      pthread_rwlock_unlock(&g_rw);
    } while (tmp < 3);

  printf("done!\n");

  return 0;
}
#else
int main(int argc, FAR char *argv[])
{
  printf("components3 examples\n");

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS3_THREADS
  threads_init();
#endif

  while (g_val < 3)
    {
      sleep(1);
    }

  printf("done!\n");

  return 0;
}
#endif
