/****************************************************************************
 * apps/mini_cannode/cannode.h
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

#ifndef __RAILAB_APPS_MINI_CANNODE_CANNODE_H
#define __RAILAB_APPS_MINI_CANNODE_CANNODE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/can/can.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CAN frames */

#define CANMSG_HEARTBEAT_ID    1
#define CANMSG_BUTTON_ID       2
#define CANMSG_LED_ID          3

#define CANMSG_HEARTBEAT_BYTES 8
#define CANMSG_BUTTON_BYTES    1

#define CANMSG_NODEID_MASK     0xf00

#define CAN_DATA_MAX 8

/* Debug prints */

#ifdef CONFIG_SERIAL
#  define PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#  define PRINTF(...)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Threads environmente */

struct cannode_env_s
{
  uint16_t nodeid;
  int fd;
};

/* Common CAN message format for chardev and socketCAN */

struct canmsg_s
{
  uint32_t id;
  uint8_t  len;
  uint8_t  data[CAN_DATA_MAX];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int can_init(void);
int can_read(int fd, FAR struct canmsg_s *msg);
int can_send(int fd, FAR struct canmsg_s *msg);

#endif /* __RAILAB_APPS_MINI_CANNODE_CANNODE_H */
