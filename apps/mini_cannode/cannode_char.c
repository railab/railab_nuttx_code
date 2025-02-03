/****************************************************************************
 * apps/mini_cannode/cannode_char.c
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

#include <fcntl.h>
#include <stdio.h>

#include <nuttx/can/can.h>

#include "cannode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAN_DEVPATH    "/dev/can0"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_init
 *
 * Description:
 *   Initialzie CAN character device interface.
 *
 ****************************************************************************/

int can_init(void)
{
  int fd;

  /* Open device */

  fd = open(CAN_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      PRINTF("failed to open %s %d\n", CAN_DEVPATH, -errno);
      return -errno;
    }

  return fd;
}

/****************************************************************************
 * Name: can_read
 *
 * Description:
 *   Read CAN data (blocking).
 *
 ****************************************************************************/

int can_read(int fd, FAR struct canmsg_s *msg)
{
  struct can_msg_s frame;
  int              ret;

  /* Read frame */

  ret = read(fd, &frame, sizeof(struct can_msg_s));
  if (ret < 0)
    {
      PRINTF("read failed %d\n", -errno);
      return -errno;
    }

  /* Convert frame to common format */

  msg->id  = frame.cm_hdr.ch_id;
  msg->len = can_dlc2bytes(frame.cm_hdr.ch_dlc);
  memcpy(msg->data, frame.cm_data, CAN_DATA_MAX);

  return ret;
}

/****************************************************************************
 * Name: can_send
 *
 * Description:
 *   Send CAN data.
 *
 ****************************************************************************/

int can_send(int fd, FAR struct canmsg_s *msg)
{
  struct can_msg_s frame;
  int              ret;

  /* Convert from common format */

  frame.cm_hdr.ch_id     = msg->id;
  frame.cm_hdr.ch_rtr    = false;
  frame.cm_hdr.ch_dlc    = can_bytes2dlc(msg->len);
#ifdef CONFIG_CAN_ERRORS
  frame.cm_hdr.ch_error  = 0;
#endif
#ifdef CONFIG_CAN_EXTID
  frame.cm_hdr.ch_extid  = msg->id;
#endif
  frame.cm_hdr.ch_tcf    = 0;
  memcpy(frame.cm_data, msg->data, CAN_DATA_MAX);

  /* Send frame */

  ret = write(fd, &frame, CAN_MSGLEN(msg->len));
  if (ret < 0)
    {
      PRINTF("write failed %d\n", -errno);
      return -errno;
    }

  return ret;
}
