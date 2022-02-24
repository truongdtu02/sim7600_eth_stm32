#ifndef _MP3_FRAME_H_
#define _MP3_FRAME_H_

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
#include "tcp_udp_stack.h"

#define TCP_PACKET_BUFF_SIZE_MAX 12 // ~60 frames //eache frame 48kbps ~ 200KB, 80 frames ~ 16KB
#define ADU_FRAME_SIZE 432 //bytes

#define MP3_BLOCK_SIZE (432 + 144*4)

#define WAIT_TCP_PACKET_BUFF_MUTEX 10 //10 ticks ~ 10ms
#define MP3_FRAME_SIZE 144 //bytes
#define NUM_OF_FRAME_IN_TCP_PACKET 5 //1ADU + 4MP3

#define min(a,b) (((a)<(b))?(a):(b))

typedef struct
{
	uint8_t type;
    uint32_t session;
    uint8_t aesMP3key[AES128_KEY_LEN];
    uint8_t volume;
    int64_t timestamp;
    uint32_t frameID;
    uint8_t numOfFrame;
    uint16_t sizeOfFirstFrame;
    uint16_t frameSize;
    uint8_t timePerFrame;
    uint8_t data[];
} __attribute__ ((packed)) MP3Struct;
//} MP3Struct;

typedef struct PacketStruct
{
    int64_t timestamp;
	uint8_t mp3Frame[432 + 4 * 144];
    int bool_isempty;
} PacketStruct;

void mp3SaveFrame(MP3Struct *mp3Packet, int len);
int mp3GetVol();
// FrameStruct *mp3GetNewFrame();
// FrameStruct *mp3GetHeadFrame();
// bool mp3CheckFrame(FrameStruct *frame);
// void mp3RemoveTcpFrame(FrameStruct *frame);

int getCurrentNumOfPacket();

#endif
