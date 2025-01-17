NuttX code samples for www.railab.me

To initialize the repository use this::

    git clone https://github.com/railab/nuttx external/nuttx
    git clone https://github.com/railab/nuttx-apps external/nuttx-apps
    ln -sr apps external/nuttx-apps/external

Apache NuttX and small systems series
=====================================

Hello, World !
--------------

Examples for www.railab.me/posts/2024/11/nuttx-and-small-systems-hello-world/

The common command to build examples is::

  cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8-mini/config/{CONFIG_NAME} -GNinja
  cmake --build build

Where ``CONFIG_NAME`` for a given example are given below:

* ``mini_hello`` - minimal "Hello, world!":

* ``mini_hello_opt`` - minimal, optimized "Hello, world!":

NuttX Core
----------

Examples for www.railab.me/posts/2024/12/nuttx-and-small-systems-core-os/

The common command to build examples is::

  cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8-mini/config/{CONFIG_NAME} -GNinja
  cmake --build build

Where ``CONFIG_NAME`` for a given example are given below:

* ``mini_core1`` - minimal NuttX image with serial port enabled:

* ``mini_core2`` - minimal NuttX image with serial port disabled:

* ``mini_core3`` - minimal NuttX image with serial port disabled and no ``/dev/null``:

* ``mini_blinky1`` - blinky with files:

* ``mini_blinky2`` - blinky with arch-specific code:

NuttX Components
----------------
             
Examples for www.railab.me/posts/2025/1/nuttx-and-small-systems-os-components/

The common command to build examples is::

  cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8-mini/config/{CONFIG_NAME} -GNinja
  cmake --build build

Where ``CONFIG_NAME`` for a given example are given below:

* Application 1 - DEFAULT_SMALL influence:

  - ``mini_components1`` - base configuration

  - ``mini_components1_env`` - environment variable support enabled

  - ``mini_components1_tim`` - POSIX timers support enabled

  - ``mini_components1_pth`` - Pthreads support enabled

  - ``mini_components1_mq`` - POSIX message queue support enabled

  - ``mini_components1_mqsys`` - System V message queue support enabled

  - ``mini_components1_osall`` - enable all OS interfaces

  - ``mini_components1_fstream`` - FILE stream support enabled

* Application 2 - poll() and epoll():

  - ``mini_components2`` - base configuration

  - ``mini_components2_busy`` - busy wait for timer

  - ``mini_components2_poll`` - poll wait for timer

  - ``mini_components2_epoll`` - epoll wait for timer

* Application 3 - thread data protection:

  - ``mini_components3`` - base configuration

  - ``mini_components3_pth`` - threads only

  - ``mini_components3_atomic`` - thread data as atomic

  - ``mini_components3_mut`` - thread data protected with mutex

  - ``mini_components3_cond`` - thread data with condition variable

  - ``mini_components3_rwlock`` - thread data protected with rwlock

* Application 4 - signaling:

  - ``mini_components4`` - base configuration

  - ``mini_components4_pth`` - notify main with thread and semaphore

  - ``mini_components4_tim`` - notify main with POSIX timer signal

  - ``mini_components4_tim_sigev`` - notify main with POSIX timer signal and SIGEV_THREAD

* Application 5 - printf and libm:

  - ``mini_components5`` - base configuration

  - ``mini_components5_float_nofpu`` - printf with floating point support and FPU disabled

  - ``mini_components5_float_fpu`` - printf with floating point support and FPU enabled

  - ``mini_components5_long`` - printf with long long support

  - ``mini_components5_libm_nx_nofpu`` - libm from NuttX with FPU disabled

  - ``mini_components5_libm_nx_fpu`` - libm from NuttX with FPU enabled

  - ``mini_components5_libm_newlib_nofpu`` - libm from Newlib with FPU disabled

  - ``mini_components5_libm_newlib_fpu`` - libm from Newlib with FPU enabled

  - ``mini_components5_libm_fixedmath`` - fixed math test

  - ``mini_components5_libm_floatmath_nofpu`` - float math test with FPU disabled and NuttX libm

  - ``mini_components5_libm_floatmath_fpu`` - float math test with FPU enabled and NuttX libm

