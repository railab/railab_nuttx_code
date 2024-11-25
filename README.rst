NuttX code samples for www.railab.me

To initialize the repository use this::

    git clone https://github.com/railab/nuttx external/nuttx
    git clone https://github.com/railab/nuttx-apps external/nuttx-apps
    ln -sr apps external/nuttx-apps/external

Apache NuttX and small systems
==============================

Hello, World !
--------------

Examples for www.railab.me/posts/2024/11/nuttx-and-small-systems-hello-world/

* minimal "Hello, world!":

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_hello -GNinja
     cmake --build build

* minimal, optimized "Hello, world!":

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_hello_opt -GNinja
     cmake --build build

NuttX Core
----------

Examples for www.railab.me/posts/2024/12/nuttx-and-small-systems-core-os/
             
* minimal NuttX image with serial port enabled:

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_core1 -GNinja
     cmake --build build

* minimal NuttX image with serial port disabled:

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_core2 -GNinja
     cmake --build build

* minimal NuttX image with serial port disabled and no ``/dev/null``:

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_core3 -GNinja
     cmake --build build

* blinky with files:

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_blinky1 -GNinja
     cmake --build build

* blinky with arch-specific code:

  .. code:: bash

     cmake -B build -S external/nuttx -DBOARD_CONFIG=../../boards/arm/stm32/nucleo-f302r8/config/mini_blinky2 -GNinja
     cmake --build build
