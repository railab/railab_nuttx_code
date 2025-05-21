/****************************************************************************
 * apps/mini_cannode/cannode_sock.c
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

#include <stdio.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <nuttx/can.h>
#include <nuttx/can/can.h>

#include "cannode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_init
 *
 * Description:
 *   Initialzie CAN socket device interface.
 *
 ****************************************************************************/

int can_init(void)
{
  struct sockaddr_can addr;
  struct ifreq        req;
  int                 s;
  int                 ret;

  /* Create sockete */

  s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0)
    {
      PRINTF("failed to open sock %d\n", -errno);
      return -errno;
    }

  /* Perform the ioctl to bring up network interface */

  memset(&req, 0, sizeof(struct ifreq));
  strlcpy(req.ifr_name, "can0", IFNAMSIZ);
  req.ifr_flags |= IFF_UP;
  ret = ioctl(s, SIOCSIFFLAGS, (unsigned long)&req);

  /* Bind socket */

  memset(&addr, 0, sizeof(addr));
  addr.can_family  = AF_CAN;
  addr.can_ifindex = 1;

  ret = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      PRINTF("bind failed %d\n", -errno);
      return -errno;
    }

  return s;
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
  struct can_frame frame;
  int              ret;

  ret = read(fd, &frame, sizeof(struct can_frame));
  if (ret < 0)
    {
      PRINTF("recvmsg failed %d\n", -errno);
      return -errno;
    }

  msg->id = frame.can_id;
  msg->len = can_dlc2bytes(frame.can_dlc);
  memcpy(msg->data, frame.data, CAN_DATA_MAX);

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
  struct can_frame frame;
  int              ret;

  /* Convert to SocketCAN frame */

  frame.can_id = msg->id;
#ifdef CONFIG_NET_CAN_EXTID
  frame.can_id |= CAN_EFF_FLAG;
#endif
  frame.can_dlc = can_bytes2dlc(msg->len);
  memcpy(frame.data, msg->data, msg->len);

  /* Send frame */

  ret = write(fd, &frame, CAN_MTU);
  if (ret < 0)
    {
      PRINTF("write failed %d\n", -errno);
      return -errno;
    }
  else if (ret != CAN_MTU)
    {
      PRINTF("write failed to send\n");
      return -EIO;
    }

  return ret;
}
