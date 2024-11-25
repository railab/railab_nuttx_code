# ##############################################################################
# apps/mini_blinky_stm32/CMakeLists.txt
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_RAILAB_MINIMAL_BLINKY2)
  nuttx_add_application(
    NAME
    blinky2
    SRCS
    blinky2_main.c
    STACKSIZE
    ${CONFIG_RAILAB_MINIMAL_BLINKY2_STACKSIZE})

  # This is Bad, Mmkay? You shouldn't do that, Mkay?
  #
  # allow #include "stm32.h" and #include "arm_internal.h"

  target_include_directories(apps_blinky2 PRIVATE ${CMAKE_SOURCE_DIR}/arch/arm/src/stm32)
  target_include_directories(apps_blinky2 PRIVATE ${CMAKE_SOURCE_DIR}/arch/arm/src/common)
endif()
