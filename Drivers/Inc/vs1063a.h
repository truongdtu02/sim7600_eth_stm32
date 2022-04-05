/****************************************Copyright (c)****************************************************
**                                      
**                                 http://www.powermcu.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               VS1003.h
** Descriptions:            The VS1003 application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              AVRman
** Created date:            2011-2-17
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

#ifndef _VS1063A_H_
#define _VS1063A_H_
												   
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f407xx.h"
#include "stdint.h"
#include "cmsis_os.h"
#include "stdbool.h"
#include "mp3Frame.h"

#define VS1063_TASK_INTERVAL 10 //10ms

#define MP3_DREQ_WAIT_CLKS 300000 //cpu_clk = 168MHz -> MP3_DREQ_WAIT_CLKS ~ 2ms
#define SPI_TIMEOUT_BYTE 50 // 50ms in worst case fCLK = 42MHZ, scale = 256
#define SPI_TIMEOUT_ARRAY 1000 //1s

#define CONFIG_SPI_CLOCKF 0xB000 // sci mul=4, sci add = 1,5

#define SPI_AUDIO_PRESCLE_RESET SPI_BAUDRATEPRESCALER_128
#define SPI_AUDIO_PRESCLE_MAIN SPI_BAUDRATEPRESCALER_16


/* Private define ------------------------------------------------------------*/
#define SPI_SPEED_HIGH    1  
#define SPI_SPEED_LOW     0

// extern SPI_HandleTypeDef hspi2;

#define DREQ_Pin GPIO_PIN_9
#define DREQ_GPIO_Port GPIOE

#define XCS_Pin GPIO_PIN_11
#define XCS_GPIO_Port GPIOE

#define XDCS_Pin GPIO_PIN_13
#define XDCS_GPIO_Port GPIOE

#define XRST_Pin GPIO_PIN_7
#define XRST_GPIO_Port GPIOE
;
/* reset */
#define MP3_Reset(x) x ? HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_RESET)

#define MP3_CCS(x) x ? HAL_GPIO_WritePin(XCS_GPIO_Port, XCS_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(XCS_GPIO_Port, XCS_Pin, GPIO_PIN_RESET)

#define MP3_DCS(x) x ? HAL_GPIO_WritePin(XDCS_GPIO_Port, XDCS_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(XDCS_GPIO_Port, XDCS_Pin, GPIO_PIN_RESET)

#define MP3_DREQ HAL_GPIO_ReadPin(DREQ_GPIO_Port, DREQ_Pin)

#define FM_SIGNAL_Pin GPIO_PIN_12
#define FM_SIGNAL_GPIO_Port GPIOC

#define MIC_SIGNAL_Pin GPIO_PIN_8
#define MIC_SIGNAL_GPIO_Port GPIOE

#define AMPLI_EN_Pin GPIO_PIN_5
#define AMPLI_EN_GPIO_Port GPIOE

#define FM_SIGNAL HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12)
#define MIC_SIGNAL HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8)

//EN OR DIS TPA3116D2
#define AMPLI_ON        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET)
#define AMPLI_OFF       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET)
#define AMPLI_IS_ON     LL_GPIO_IsOutputPinSet(GPIOE, LL_GPIO_PIN_5)

#define SDI_ENABLE      MP3_DCS(0); MP3_CCS(1);
#define SDI_DISABLE     MP3_DCS(1); MP3_CCS(0);
	
#define SCI_ENABLE      MP3_CCS(0); MP3_DCS(1);
#define SCI_DISABLE     MP3_CCS(1); MP3_DCS(0);


#define VS_WRITE_COMMAND 	0x02
#define VS_READ_COMMAND 	0x03

/* VS1003 */
#define SPI_MODE        	0x00   
#define SPI_STATUS      	0x01   
#define SPI_BASS        	0x02   
#define SPI_CLOCKF      	0x03   
#define SPI_DECODE_TIME 	0x04   
#define SPI_AUDATA      	0x05   
#define SPI_WRAM        	0x06   
#define SPI_WRAMADDR    	0x07   
#define SPI_HDAT0       	0x08   
#define SPI_HDAT1       	0x09 
#define SPI_AIADDR      	0x0a   
#define SPI_VOL         	0x0b   
#define SPI_AICTRL0     	0x0c   
#define SPI_AICTRL1     	0x0d   
#define SPI_AICTRL2     	0x0e   
#define SPI_AICTRL3     	0x0f 
#define AICTRL3_ADC_LEFT      2
#define AICTRL3_ADC_RIGHT     3
#define AICTRL3_ADC_MASK      0xFFF8 //just set 3 LSB

#define SM_DIFF         	0x01   
#define SM_JUMP         	0x02   
#define SM_RESET        	0x04   
#define SM_OUTOFWAV     	0x08   
#define SM_PDOWN        	0x10   
#define SM_TESTS        	0x20   
#define SM_STREAM       	0x40   
#define SM_PLUSV        	0x80   
#define SM_DACT         	0x100   
#define SM_SDIORD       	0x200   
#define SM_SDISHARE     	0x400   
#define SM_SDINEW       	0x800   
#define SM_ADPCM        	0x1000   
#define SM_ADPCM_HP     	0x2000 	

#define PLAYMOD_ADDR          0xC0C9 //addr write to WRAMADDR
#define ADD_MIXER_GAIN_ADDR   0xC0CD
#define ADD_MIXER_CONFIG_ADDR 0xC0CE
#define PLAYMOD_MIC_FM        1 | (1<<3)     //enable admixer
#define PLAYMOD_MP3        1                       //disable admixer
/* Private function prototypes -----------------------------------------------*/
void VS1063_Init(void);

void VS1063_SoftReset(void);
void VS1063_PlayBeep(void);
int waitMp3DREQ();
void VS1063_PlayMP3_Task();

void VS1063_PlaySong();

void VS1063_PlayBuff();

void VS1063_PrintBuff();

bool VS1063_IsRuning();
#endif

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

