#include "flash.h"

uint32_t old_addr = 0;

uint32_t Flash_Write_Data (uint32_t StartSectorAddress, uint32_t *Data, uint16_t numberofwords)
{
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError;
	int sofar=0;


	 /* Unlock the Flash to enable the flash control register access *************/
	  HAL_FLASH_Unlock();

	  /* Erase the user Flash area */

	  /* Get the number of sector to erase from 1st sector */

	  uint32_t StartSector = GetSector(StartSectorAddress);
	  uint32_t EndSectorAddress = StartSectorAddress + numberofwords*4;
	  uint32_t EndSector = GetSector(EndSectorAddress);

	  /* Fill EraseInit structure*/
	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	  EraseInitStruct.Sector        = StartSector;
	  EraseInitStruct.NbSectors     = (EndSector - StartSector) + 1;

	  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	     you have to make sure that these data are rewritten before they are accessed during code
	     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	     DCRST and ICRST bits in the FLASH_CR register. */
	  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	  {
		  return HAL_FLASH_GetError ();
	  }

	  /* Program the user Flash area word by word
	    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	   while (sofar<numberofwords)
	   {
	     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartSectorAddress, Data[sofar]) == HAL_OK)
	     {
	    	 StartSectorAddress += 4;  // use StartPageAddress += 2 for half word and 8 for double word
	    	 sofar++;
	     }
	     else
	     {
	       /* Error occurred while writing data in Flash memory*/
	    	 return HAL_FLASH_GetError ();
	     }
	   }

	  /* Lock the Flash to disable the flash control register access (recommended
	     to protect the FLASH memory against possible unwanted operation) *********/
	  HAL_FLASH_Lock();

	   return 0;
}

void Flash_Read_Data (uint32_t StartPageAddress, uint32_t *RxBuf, uint16_t numberofwords)
{
	while (1)
	{
		*RxBuf = *(__IO uint32_t *)StartPageAddress;
		StartPageAddress += 4;
		RxBuf++;
		if (!(numberofwords--)) break;
	}
}



bool Flash_Write_Word(uint32_t addr, uint32_t *data)
{
    if(addr < LOW_ADDR_LIMIT || addr > HIGH_ADDR_LIMIT) return false;
    
    HAL_FLASH_Unlock();
    bool isError = true;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *data) == HAL_OK)
    {
        isError = false;
    }
    HAL_FLASH_Lock();
    if(!isError)
    {
        return true;
    }
    else
    {
        HAL_FLASH_GetError();
        return false;
    }
}

uint32_t Flash_Read_Word(uint32_t addr)
{
    if(addr < LOW_ADDR_LIMIT || addr > HIGH_ADDR_LIMIT) return 0;
    uint32_t res = *(__IO uint32_t *)addr;

    return res;
}

//erase sector
void Flash_Reset_Sector(uint32_t addr)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SECTORError;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Erase the user Flash area */

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    if(addr == SECTOR6_ADDR)
        EraseInitStruct.Sector = FLASH_SECTOR_6;
    else if(addr == SECTOR7_ADDR)
        EraseInitStruct.Sector = FLASH_SECTOR_7;
    else
        return;
    EraseInitStruct.NbSectors = 1;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	     you have to make sure that these data are rewritten before they are accessed during code
	     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	     DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
    {
    	HAL_FLASH_GetError();
        return;
    }
}

//erase to debug
void log_erase()
{
    Flash_Reset_Sector(SECTOR6_ADDR);
    Flash_Reset_Sector(SECTOR7_ADDR);
}

//in the first time, we will check first word of two sector, if those != 0xFF FF FF FF, that mean we need to read log, and reset this for the next debug
uint32_t countTime = 0;
void log_write(const char* data)
{
    if(data == NULL || strlen(data) != 4) return;

    if(old_addr == 0) //first time, after reboot
    {
        //get first word, sector 6, 7
        uint32_t firstW = Flash_Read_Word(SECTOR6_ADDR);
        uint32_t secondW = Flash_Read_Word(SECTOR7_ADDR);
        if(firstW != 0xFFFFFFFF || secondW != 0xFFFFFFFF) //can't write, need read and reset by stm32-sl link utility
        {
            return;
        }
        //else first write, write to sector 6
        Flash_Write_Word(SECTOR6_ADDR, &countTime);
        countTime++;
        Flash_Write_Word(SECTOR6_ADDR + 4, data);
        old_addr = SECTOR6_ADDR + 8;
        return;
    }
    if(old_addr == SECTOR7_ADDR)
    {
        //erase first
        Flash_Reset_Sector(SECTOR7_ADDR);

        Flash_Write_Word(SECTOR7_ADDR, &countTime);
        countTime++;
        Flash_Write_Word(SECTOR7_ADDR + 4, data);
        old_addr = SECTOR7_ADDR + 8;
        return;
    }
    else if(old_addr == SECTOR8_ADDR)
    {
        //erase first
        Flash_Reset_Sector(SECTOR6_ADDR);

        Flash_Write_Word(SECTOR6_ADDR, &countTime);
        countTime++;
        Flash_Write_Word(SECTOR6_ADDR + 4, data);
        old_addr = SECTOR6_ADDR + 8;
        return;
    }
    else //normal write
    {
        Flash_Write_Word(old_addr, data);
        old_addr +=4;
    }
}
