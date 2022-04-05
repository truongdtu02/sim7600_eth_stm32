
#ifndef TCP_UDP_STACK_H_
#define TCP_UDP_STACK_H_

#include "mp3Frame.h"
#include "main.h"
#include "stm32f407xx.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include <stdio.h>
#include "rsa.h"
#include "AES_128.h"
#include "sim7600_uart_dma.h"
#include "md5.h"
#include "ethLAN.h"

#define TCP_BUFF_LEN 10000
#define MD5_LEN 16 //16B

#define CONNECT_TO_HOST_COUNT_MAX 10

/*
4B      len

16B     MD5

256B    pubkey
*/

//TcpPacketStruct
#define POS_OF_LEN          0
#define SIZE_OF_LEN         2 //2B len of packet

#define POS_OF_MD5          2 //right after len filed
#define SIZE_OF_MD5         16 //16B MD5

#define POS_OF_PAYLOAD      18 //position of payload of packet
#define SIZE_OF_PUBKEY      256 //bytes

#define HEADER_LEN          (SIZE_OF_LEN + SIZE_OF_MD5)

#define RSA_PACKET_LEN (RSA_BLOCK_LEN + SIZE_OF_LEN) //size of rsa pubkey

#define TLS_HANDSHAKE_TIMEOUT 20000 //20s
#define TIMER_INTERVAL 2000 //10s
#define SEND_STATUS_INTERVAL 300 // 60 * TIMER_INTERVAL 600s
#define KEEP_ALIVE_INTERVAL 20 // 6 * TIMER_INTERVAL 40s
#define NTP_INTERVAL 300 // 390 * TIMER_INTERVAL 3900s
#define RTT_NTP_MAX 100

#define PACKET_TCP_HEADER_LENGTH (4 + 16 + 1) // 4 byte len, 16 byte md5, 1B type

#define DEVICE_LEN_MAX 50

#define MP3_PACKET_HEADER_LEN (1 + 4 + 16 + 1 + 8 + 4 + 1 + 2 + 2 + 1) //not inlcude 4B len, 16B md5
#define MP3_PACKET_HEADER_LEN_BEFORE_VOLUME (1 + 4 + 16)

#define UDP_BUFF_LEN 14
#define UDP_PACKET_LEN 28

typedef struct
{
    uint16_t len; //encoded (reserved)
	uint8_t md5[MD5_LEN];
    uint8_t payload[];
} __attribute__ ((packed)) PacketTCPStruct;
// } PacketTCPStruct;

typedef struct
{
	uint8_t md5[MD5_LEN];
    int64_t serverTime;
    int32_t tmp;
    uint32_t clientTime;
} __attribute__ ((packed)) NTPStruct;

typedef struct
{
    int64_t serverTime;
    uint32_t clientTime;
    uint16_t checksum;
} __attribute__ ((packed)) NTPStruct2;

enum SendRecvPackeTypeEnum { StatusRecvEnum, PacketMP3Enum };

void TCP_Request();

void TCP_Packet_Analyze(uint8_t *tcpPacket, int length);
void UDP_Packet_Analyze(uint8_t* data, int len);
void TCP_UDP_Stack_Init(osEventFlagsId_t eventID, int successFlag, int errorFlag, bool IsETH);//reset before connect TCP beggin, and get flags
void TCP_UDP_Stack_Release();
bool Is_TCP_LTSHanshake_Connected(); //get status
void TCP_Timer_Callback(void* argument);
// void TCP_UDP_Notify(int flagsEnum);

//get UTC NTP time
int64_t TCP_UDP_GetNtpTime();

bool CheckMD5(PacketTCPStruct *packet);
bool CheckSaltACK(uint8_t *data, int len);

enum SaltEnum { Add, Sub};
void ConvertTextWithSalt(uint8_t *data, int offset, int len, enum SaltEnum saltType);

//static byte type

#endif /* TCP_UDP_STACK_H_ */
