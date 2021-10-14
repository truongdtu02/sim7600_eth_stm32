#ifndef _FLASH_H_
#define _FLASH_H_

#include "main.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

//write log on sector 6, 7
#define SECTOR6_ADDR 0x08040000
#define SECTOR7_ADDR 0x08060000
#define SECTOR8_ADDR 0x08080000

//apply for write and read by word (4B)
#define LOW_ADDR_LIMIT 0x08040000
#define HIGH_ADDR_LIMIT 0x08080000 - 4

// uint32_t Flash_Write_Data (uint32_t StartSectorAddress, uint32_t *Data, uint16_t numberofwords);
// void Flash_Read_Data (uint32_t StartPageAddress, uint32_t *RxBuf, uint16_t numberofwords);

//write 4B string


#endif
