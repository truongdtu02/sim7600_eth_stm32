//run with RTOS

#include "vs1063a.h"
extern SPI_HandleTypeDef hspi2;
#define spiAudio hspi2
#define vs1063Delay HAL_Delay
bool errorVS1063A = true; //for first time init
int volMul = 100;		  //config in flash by ep8266
int mainVol = 80;

#define VSLOG_WRITE printf

static uint8_t VS1063_SPI_ReadByte(void)
{
	uint8_t rxData;

	HAL_StatusTypeDef res = HAL_SPI_Receive(&spiAudio, &rxData, 1, SPI_TIMEOUT_BYTE);
	if (res != HAL_OK)
	{
		VSLOG_WRITE("HAL_SPI_Receive Error\n");
		errorVS1063A = true;
	}

	return rxData;
}

static void VS1063_SPI_WriteByte(uint8_t TxData)
{
	HAL_StatusTypeDef res = HAL_SPI_Transmit(&spiAudio, &TxData, 1, SPI_TIMEOUT_BYTE);
	if (res != HAL_OK)
	{
		VSLOG_WRITE("HAL_SPI_Transmit Error\n");
		errorVS1063A = true;
	}
}

static void VS1063_SPI_WriteArray(uint8_t *TxData, int len)
{
	VSLOG_WRITE("VS1063_SPI_WriteArray\n");
	HAL_StatusTypeDef res = HAL_SPI_Transmit(&spiAudio, TxData, len, HAL_MAX_DELAY);
	if (res != HAL_OK)
	{
		VSLOG_WRITE("HAL_SPI_Transmit Error\n");
		errorVS1063A = true;
	}
}

//return 1 ~ success, 0 ~ , wait until MP3_DREQ == 1
int waitMp3DREQ()
{
	VSLOG_WRITE("waitMp3DREQ\n");
	int try = MP3_DREQ_WAIT_CLKS;
	while (try-- && MP3_DREQ == 0)
		;

	if (try <= 0)
	{
		errorVS1063A = true;
		return 0;
	}

	return 1;
}

void VS1063_WriteReg(uint8_t reg, uint16_t value)
{
	VSLOG_WRITE("VS1063_WriteReg\n");
	waitMp3DREQ();

	SCI_ENABLE

	VS1063_SPI_WriteByte(VS_WRITE_COMMAND);
	VS1063_SPI_WriteByte(reg);
	VS1063_SPI_WriteByte(value >> 8);
	VS1063_SPI_WriteByte(value);

	SCI_DISABLE
}

uint16_t VS1063_ReadReg(uint8_t reg)
{
	uint16_t value;
	VSLOG_WRITE("VS1063_ReadReg\n");

	waitMp3DREQ();

	SCI_ENABLE

	VS1063_SPI_WriteByte(VS_READ_COMMAND);
	VS1063_SPI_WriteByte(reg);
	value = VS1063_SPI_ReadByte();
	value = value << 8;
	value |= VS1063_SPI_ReadByte();

	SCI_DISABLE

	return value;
}

void VS1063_ResetDecodeTime(void)
{
	VSLOG_WRITE("VS1063_ResetDecodeTime\n");
	VS1063_WriteReg(SPI_DECODE_TIME, 0x0000);
}

uint16_t VS1063_GetDecodeTime(void)
{
	VSLOG_WRITE("VS1063_GetDecodeTime\n");
	return VS1063_ReadReg(SPI_DECODE_TIME);
}

void VS1063_SoftReset(void)
{
	VSLOG_WRITE("VS1063_SoftReset\n");
	VS1063_WriteReg(SPI_MODE, (SM_SDINEW | SM_RESET)); /* */
	osDelay(2);										   /* 1.35ms */

	int retry = 5;
	while (VS1063_ReadReg(SPI_CLOCKF) != CONFIG_SPI_CLOCKF)
	{
		VS1063_WriteReg(SPI_CLOCKF, CONFIG_SPI_CLOCKF);
		osDelay(1);
		if (retry-- < 0)
		{
			printf("SPI_CLOCKF Set Error\r\n");
			errorVS1063A = true;
			return;
		}
	}

	VS1063_SetVol(mainVol);
}

//input vol 0-100 ~ %, convert 0-100 ~ 0xFE-0x00
void VS1063_SetVol(int vol)
{
	vol = vol * volMul / 100;
	if (vol >= 100)
	{
		vol = 0x00;
	}
	else if (vol <= 0)
	{
		vol = 0xFE;
	}
	else
	{
		vol = (100 - vol) * 0xFE / 100;
	}
	vol = vol | (vol << 8); //convert to 2 channels
	VSLOG_WRITE("vs1063 set vol %X", vol);
	VS1063_WriteReg(SPI_VOL, vol);
}

// void VS1063_Record_Init(void)
// {
//   uint8_t retry;
//
//   /* */
//   while( VS1063_ReadReg(SPI_CLOCKF) != 0x9800 )
//   {
// 	  VS1063_WriteReg(SPI_CLOCKF,0x9800);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
//
//   while( VS1063_ReadReg(SPI_BASS) != 0x0000 )
//   {
// 	  VS1063_WriteReg(SPI_CLOCKF,0x0000);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
//
//   /* Set sample rate divider=12 */
//   while( VS1063_ReadReg(SPI_AICTRL0) != 0x0012 )
//   {
// 	  VS1063_WriteReg(SPI_AICTRL0,0x0012);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
//
//   /* AutoGain OFF, reclevel 0x1000 */
//   while( VS1063_ReadReg(SPI_AICTRL1) != 0x1000 )
//   {
// 	  VS1063_WriteReg(SPI_AICTRL1,0x1000);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
//
//   /* RECORD,NEWMODE,RESET */
//   while( VS1063_ReadReg(SPI_MODE) != 0x1804 )
//   {
// 	  VS1063_WriteReg(SPI_MODE,0x1804);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
//
//   while( VS1063_ReadReg(SPI_CLOCKF) != 0x9800 )
//   {
// 	  VS1063_WriteReg(SPI_CLOCKF,0x9800);
// 	  HAL_Delay(2);                        /* 1.35ms */
// 	  if( retry++ > 100 )
// 	  {
// 	      break;
// 	  }
//   }
// }

uint8_t szBeepMP3[] = // 432
	{
		0xff, 0xe2, 0x19, 0xc0, 0xd4, 0x80, 0x00, 0x0a, 0x61, 0x76, 0x72, 0xe9, 0x41, 0x30, 0x01, 0x0d,
		0xbe, 0x90, 0xcc, 0x13, 0x0f, 0xc6, 0xe3, 0xf8, 0xdf, 0xfe, 0x05, 0xfc, 0x0b, 0xc0, 0xab, 0xc8,
		0x0b, 0xff, 0xff, 0xf6, 0x8b, 0xbf, 0xe2, 0x23, 0x2c, 0x10, 0x82, 0x18, 0xf7, 0x7a, 0xd7, 0x77,
		0xad, 0x11, 0x8e, 0x61, 0x04, 0x00, 0x3a, 0xf0, 0xff, 0xb0, 0x04, 0xb1, 0x00, 0x00, 0x06, 0x59,
		0xc3, 0x99, 0x00, 0x00, 0x70, 0x0b, 0x80, 0x00, 0xff, 0xe2, 0x19, 0xc0, 0xc6, 0xe8, 0x07, 0x0b,
		0x11, 0x9e, 0xee, 0xf9, 0x81, 0x68, 0x02, 0x01, 0xb8, 0x00, 0x00, 0x39, 0x42, 0x12, 0xff, 0xff,
		0x70, 0x0f, 0xff, 0xae, 0xbf, 0xab, 0xfe, 0xa4, 0x1b, 0xf0, 0xe5, 0x37, 0xd6, 0x1d, 0x7e, 0xa6,
		0x7f, 0xe3, 0x30, 0xdf, 0xfe, 0x33, 0x0e, 0xbc, 0xb1, 0x97, 0xf5, 0x07, 0x7b, 0x27, 0xff, 0xff,
		0xff, 0x25, 0x5d, 0xb8, 0xce, 0x9b, 0x0a, 0x7a, 0x9b, 0x96, 0x81, 0xaf, 0x92, 0x02, 0x83, 0x97,
		0xff, 0xe2, 0x19, 0xc0, 0x06, 0x63, 0x13, 0x0b, 0x79, 0x7e, 0x90, 0x21, 0xc0, 0xd8, 0x00, 0xb4,
		0xa6, 0xd4, 0xa6, 0x97, 0x1f, 0xff, 0xfe, 0x63, 0x84, 0xa9, 0x4a, 0x93, 0xe8, 0xaa, 0xe0, 0x7a,
		0xa0, 0xe5, 0xaa, 0x4e, 0xa6, 0xb2, 0xea, 0xbc, 0x77, 0xf5, 0x00, 0xdd, 0xb0, 0x18, 0x03, 0xff,
		0xf5, 0x90, 0x1e, 0x72, 0x2e, 0x6f, 0xff, 0xfe, 0x7c, 0xc7, 0xff, 0xa0, 0x81, 0x4c, 0x52, 0x60,
		0x64, 0x4f, 0x09, 0x88, 0xcd, 0x93, 0xe6, 0xff, 0xff, 0xe2, 0x19, 0xc0, 0xcd, 0x5a, 0x1e, 0x0b,
		0x69, 0x76, 0xba, 0xe0, 0x08, 0x68, 0x6c, 0xf9, 0x99, 0xba, 0x41, 0xfa, 0x00, 0x61, 0x80, 0x2d,
		0xe8, 0xa0, 0x33, 0x05, 0x77, 0x35, 0x4f, 0x1b, 0x5b, 0x38, 0x00, 0x07, 0x1f, 0xf9, 0x85, 0x7f,
		0xcc, 0x3f, 0x3f, 0x0a, 0xf9, 0xaf, 0xf8, 0x43, 0xff, 0xff, 0x35, 0xd6, 0xe1, 0x2b, 0x8d, 0x21,
		0x39, 0x00, 0x64, 0x69, 0x05, 0x74, 0xf0, 0x77, 0x9d, 0x5b, 0x7f, 0xe2, 0xdf, 0x2c, 0x25, 0xf4,
		0xff, 0xe2, 0x19, 0xc0, 0x22, 0x06, 0x29, 0x0f, 0x09, 0x7a, 0xa2, 0x38, 0x08, 0x5e, 0x6e, 0xe8,
		0x00, 0x3c, 0x2d, 0x60, 0xe5, 0x3c, 0x71, 0x77, 0xba, 0x12, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x43,
		0xdf, 0x0d, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe2, 0x19, 0xc0, 0xbc, 0xd1, 0x25, 0x00,
		0x00, 0x02, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// //24B mono, mpeg 2, 8kbps
// uint8_t mute1framse[] = {0xFF, 0xF3, 0x14, 0xC4, 0x16, 0x00, 0x00, 0x03,
// 						 0x48, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x45, 0x33,
// 						 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41};
// //1200B
// uint8_t mute50framse[] = {0xFF, 0xF3, 0x14, 0xC4, 0x16, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x21, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x2C, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x37, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x42, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x4D, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x58, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x63, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x6E, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x79, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x84, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x8F, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0x9A, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xA5, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xB0, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xBB, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xC6, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xD1, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xDC, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xE7, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF2, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x4C, 0x41, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x4D, 0x45, 0x33, 0x2E, 0x31, 0x30, 0x30, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xFF, 0xF3, 0x14, 0xC4, 0xF4, 0x0, 0x0, 0x3, 0x48, 0x0, 0x0, 0x0, 0x0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
//
// void VS1063_PlayBeep_DMA(void)
// {
//     MP3_DCS(0);
//     MP3_CCS(1);
// 	HAL_SPI_Transmit_DMA(hspiVS1063, szBeepMP3, sizeof(szBeepMP3));
//
// }
//
// void VS1063_Play_Data_DMA(uint8_t *data, int length)
// {
//     MP3_DCS(0);
//     MP3_CCS(1);
// 	HAL_SPI_Transmit_DMA(hspiVS1063, data, length);
// }
//
// void VS1063_Stop_DMA()
// {
// 	HAL_SPI_DMAStop(hspiVS1063);
// }
//
// void VS1063_Play_1frameMute_DMA()
// {
//     MP3_DCS(0);
//     MP3_CCS(1);
// 	HAL_SPI_Transmit_DMA(hspiVS1063, mute1framse, 24);
// }
//
// void DREQ_VS1063_IRQhandler()
// {
// 	if (!MP3_DREQ) //falling
// 	    {
// 	      //pause DMA
// 	      HAL_SPI_DMAPause(hspiVS1063);
// 	    }
// 	    else //rising
// 	    {
// 	      //resume DMA
// 	      HAL_SPI_DMAResume(hspiVS1063);
// 	    }
// }
//

void VS1063_Init(void)
{
	if (!errorVS1063A)
		return;

	VSLOG_WRITE("VS1063_Init\n");

	//change spi baud
	spiAudio.Init.BaudRatePrescaler = SPI_AUDIO_PRESCLE_RESET;
	if (HAL_SPI_Init(&spiAudio) != HAL_OK)
	{
		Error_Handler();
	}

	MP3_Reset(0);
	osDelay(100);
	MP3_Reset(1);

	MP3_DCS(1);
	MP3_CCS(1);

	osDelay(100);

	//config
	VS1063_WriteReg(SPI_MODE, SM_SDINEW | SM_RESET); /* */
	osDelay(2);							  /* 1.35ms */

	int retry = 5;
	while (VS1063_ReadReg(SPI_CLOCKF) != CONFIG_SPI_CLOCKF)
	{
		VS1063_WriteReg(SPI_CLOCKF, CONFIG_SPI_CLOCKF);
		osDelay(1);
		if (retry-- < 0)
		{
			printf("SPI_CLOCKF Set Error\r\n");
			errorVS1063A = true;
			return;
		}
	}

	//change spi baud
	//	spiAudio.Init.BaudRatePrescaler = SPI_AUDIO_PRESCLE_MAIN;
	//	if (HAL_SPI_Init(&spiAudio) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}

	VS1063_SetVol(mainVol);
	errorVS1063A = false;
}

void VS1063_PlayBeep(void)
{
	int in;

	SDI_ENABLE

	//	VS1063_SPI_WriteArray(szBeepMP3, sizeof(szBeepMP3));
	for (in = 0; in < sizeof(szBeepMP3); in++)
		VS1063_SPI_WriteByte(szBeepMP3[in]);

	SDI_DISABLE
}

//send len byte of data frame to vs1063
void VS1063_PlayMP3(uint8_t *data, int len)
{
	SDI_ENABLE

	VS1063_SPI_WriteArray(data, len);

	SDI_DISABLE
}

void VS1063_PlayMp3Frame()
{
	static int pendingOffset = 0;
	static FrameStruct *curFrame = NULL;
	VSLOG_WRITE("*");
	for (;;)
	{
		if (curFrame == NULL)
		{
			curFrame = mp3GetNewFrame();
		}

		if(curFrame == NULL) return; //have noting to play

		//check real-time, if pendingOffset == 0
		if(pendingOffset == 0 && !mp3CheckFrame(curFrame))
		{
			curFrame = NULL;
			return;
		}
		
		if(MP3_DREQ == 1) //can send data
		{
			VSLOG_WRITE("+");
			int remainLen = curFrame->len - pendingOffset;
			if(remainLen >= 32)
			{
				VS1063_PlayMP3(curFrame->data + pendingOffset, 32);
				pendingOffset += 32;
			}
			else if(remainLen >= 0)
			{
				if(remainLen > 0)
					VS1063_PlayMP3(curFrame->data + pendingOffset, remainLen);
				pendingOffset = 0;
				mp3RemoveFrame(curFrame);
				curFrame = NULL;
			}
			// else if(remainLen == 0)
			// {
			// 	pendingOffset = 0;
			// 	mp3RemoveFrame(curFrame);
			// 	curFrame = NULL;
			// }
			else // <0 something wrong
			{
				errorVS1063A = true;
				VSLOG_WRITE("VS1063_PlayMp3Frame error\n");
				return;
			}
			continue;
		}
		else
		{
			return;
		}
	}
}

void VS1063_PlayMP3_Task()
{
	for (;;)
	{
		//init , reset if has error
		VS1063_Init();

		//read pin MIC, FM signal, config input audio

		//config vol
		if (mp3GetVol() != mainVol)
		{
			mainVol = mp3GetVol();
			VS1063_SetVol(mainVol);
		}

		//play mp3
		 VS1063_PlayMp3Frame();

//		VS1063_PlayBeep();

		 osDelay(VS1063_TASK_INTERVAL);
//		osDelay(500);
	}
}
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
