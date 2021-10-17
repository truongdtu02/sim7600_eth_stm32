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

#define MP3_NUM_OF_FRAME_MAX 80 //eache frame 48kbps ~ 200KB, 80 frames ~ 16KB

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

// typedef struct
// {
// 	FrameStruct *prevFrame, *nextFrame;
//     int64_t timestamp;
//     int len;
//     uint32_t id;
//     uint32_t session;
//     bool isTail; //this frame can remove
//     uint8_t *data;
// } FrameStruct;

struct Frame_Struct
{
    int64_t timestamp;
	struct Frame_Struct *prevFrame;
	struct Frame_Struct *nextFrame;
    int len;
    uint32_t id;
    uint32_t session;
    bool isTail; //this frame can remove
    uint8_t *data;
};

typedef struct Frame_Struct FrameStruct;

void mp3GetFrame(MP3Struct *mp3Packet, int len);
int mp3GetVol();
FrameStruct *mp3GetNewFrame();
bool mp3CheckFrame(FrameStruct *frame);
void mp3RemoveTcpFrame(FrameStruct *frame);

#endif
