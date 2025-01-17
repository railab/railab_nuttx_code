/****************************************************************************
 * apps/mini_components5/componets4_main.c
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
#include <math.h>
#include <fixedmath.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: components5_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("components5 examples\n");

  while (1)
    {
#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS5_LIBC_FLOAT
      float f1 = 0.1f;
      float f2 = 0.2f;
      float f3 = 0.3f;

#  ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS5_LIBM
      /* Do some math before print */

      f1 = sin(f2);
      f2 = cos(f1);
      f3 = powf(f1, f2);
      f3 = fabsf(f3);
      f3 = sqrtf(f3);
      f1 = expf(f3);
      f2 = asinf(f3);
      f1 = acosf(f3);
      f1 = tanhf(f2);
      f2 = logf(f1);
      f3 = atan2f(f1, f2);
#  endif

      /* Print floats */

      printf("float %.2f %.2f %.2f\n", f1, f2, f3);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS5_LIBC_LONGLONG
      uint64_t x1 = 0xdeadbeefdead;
      uint64_t x2 = 0xbeafdeadbeef;

      /* Print longs */

      printf("long %" PRIx64 " %" PRIx64 "\n", x1, x2);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS5_FIXEDMATH
      volatile b16_t b1 = ftob16(0.1f);
      volatile b16_t b2 = ftob16(0.2f);
      volatile b16_t b3 = ftob16(0.3f);

      b3 = b16sin(b1);
      b2 = b16cos(b1);
      b1 = b16atan2(b3, b2);
      b2 = b16sqr(b1);
      b3 = b16mulb16(b1, b2);
      b1 = b16divb16(b3, b2);
#endif

#ifdef CONFIG_RAILAB_MINIMAL_COMPONENTS5_FLOATMATH
      volatile float fl1 = 0.1f;
      volatile float fl2 = 0.2f;
      volatile float fl3 = 0.3f;
      fl3 = sinf(fl1);
      fl2 = cosf(fl1);
      fl1 = atan2f(fl3, fl2);
      fl2 = sqrtf(fl1);
      fl3 = fl1 * fl2;
      fl1 = fl3 / fl2;
#endif

      /* Wait some time */

      sleep(1);
    }

  return 0;
}
