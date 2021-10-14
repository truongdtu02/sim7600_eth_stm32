#ifndef _ETH_LAN_
#define _ETH_LAN_

#include "mp3Frame.h"
#include "main.h"
#include "stm32f407xx.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "tcp_udp_stack.h"
#include "sim7600_uart_dma.h"

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/dns.h"
#include "lwip/api.h"

#define RECONNECT_INTERVAR_ETH 10000 //10s

typedef struct  {
  osEventFlagsId_t EventID;
  uint8_t type; // 1: send TCP packet, 2 send UDP packet
  const uint8_t *ptr;
  int len;
  int timeout;
} sendEthPack;

enum connectEthFlagEnum {reConEthEnum, errConEthEnum, doneTLSEthEnum, errorEthEnum}; //in sim7600 .h

void ethConnectTask();
void ethSendTask();
void ethRecvTask();

bool ethSendIP(int type, uint8_t *data, int len);


#endif _ETH_LAN_
