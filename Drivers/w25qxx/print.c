/*
    Author: truongbom
    Description: log_function specific for stm32 and w25qxx
*/

#include "main.h"
#include "cmsis_os.h"
#include "print.h"
#include "w25qxx.h"
#include <stdarg.h> //va_list, va_start, va_end
#include <stdio.h> //vsprintf,printf
#include <stdint.h> //int32_t, int64_t
#include <string.h> //strlen

#define SECTOR_FULL_LIMIT 2
w25qxx_t *p_w25qxx;
int SECTOR_FREE_LIMIT;

extern osMutexId_t spiFlash_mtID;  //use for write to flash and log do erase

//free ~ empty
int sector_empty_cnt = 0, sector_full_cnt = 0, sector_wr_addr, sector_erase_addr, block_erase_addr;
int sector_last_addr = -1, page_addr_write_max;
uint32_t flash_size, byte_wr_addr;
int spiFlashMutexTimeoutCnt = 0;

void my_printf(const char *fmt, ...) {
    va_list args;
    char log_buff[100] = {'a'};

    va_start(args, fmt);
    vsprintf(log_buff, fmt, args);
    va_end(args);

    int len = strlen(log_buff);

    printf("%s", log_buff);
}

//suspend erase, before thread which use log
void log_prepare() {
    if(W25qxx_IsBusy()) {
        W25qxx_Suspend();
    }
}

int write_to_flash_suspend_cnt = 0;
void write_to_flash(char *data, int len) {
    int wrote_bytes = 0;
    uint32_t byte_offset_write, page_addr_write, res;

    // osStatus_t status = osMutexAcquire(spiFlash_mtID, 1);

    // if(status != osOK) {
    //     spiFlashMutexTimeoutCnt++;
    //     return;
    // }

    if(W25qxx_IsBusy()) {
        W25qxx_Suspend();
        W25qxx_WaitBusy(0);
        write_to_flash_suspend_cnt++;
    }

    while(len > 0) {
        page_addr_write = byte_wr_addr / p_w25qxx->PageSize;
        byte_offset_write = byte_wr_addr % p_w25qxx->PageSize;
        if(page_addr_write > page_addr_write_max) {
            page_addr_write = 0;
            byte_wr_addr = byte_offset_write;
        }
        res = W25qxx_WritePage_Quick(data + wrote_bytes, page_addr_write, byte_offset_write, len);
        wrote_bytes += res;
        len -= res;
        byte_wr_addr += res;
        byte_wr_addr %= flash_size;
    }
    // osMutexRelease(spiFlash_mtID);

    // W25qxx_ReadStatusRegister(1);
    // W25qxx_ReadStatusRegister(2);
    // W25qxx_ReadStatusRegister(3);
}

void log_to_flash(const char *fmt, ...) {
    char buff[100];
    int len;
    va_list args;
    int wrote_bytes = 0;
    uint32_t byte_offset_write, page_addr_write, res;

    osStatus_t status = osMutexAcquire(spiFlash_mtID, 1);
    if(status != osOK) {
        spiFlashMutexTimeoutCnt++;
        return;
    }

    if(W25qxx_IsBusy()) {
        W25qxx_Suspend();
        write_to_flash_suspend_cnt++;
    }

    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);

    len = strlen(buff); // +1 for /0 at the end of string

    while(len > 0) {
        page_addr_write = byte_wr_addr / p_w25qxx->PageSize;
        byte_offset_write = byte_wr_addr % p_w25qxx->PageSize;
        if(page_addr_write > page_addr_write_max) {
            page_addr_write = 0;
            byte_wr_addr = byte_offset_write;
        }
        res = W25qxx_WritePage_Quick(buff + wrote_bytes, page_addr_write, byte_offset_write, len);
        wrote_bytes += res;
        len -= res;
        byte_wr_addr += res;
        byte_wr_addr %= flash_size;
    }

    sector_wr_addr = byte_wr_addr / p_w25qxx->SectorSize;

    osMutexRelease(spiFlash_mtID);
}

//check sector status of flash
int log_init() {
    int i, bool_got_last_sector = 0, bool_sector_empty, j;

    p_w25qxx = W25qxx_Get();

    //debug
    // p_w25qxx->SectorCount = 4;
    //

    flash_size = p_w25qxx->SectorCount * p_w25qxx->SectorSize;
    page_addr_write_max = flash_size / p_w25qxx->PageSize - 1;

    // SECTOR_FREE_LIMIT = p_w25qxx->SectorCount - SECTOR_FULL_LIMIT;
    SECTOR_FREE_LIMIT = 64;

    for(i = 0; i < p_w25qxx->SectorCount; i++) {
        bool_sector_empty = W25qxx_IsEmptySector(i, 0, 0);

        //get last sector full
        if(!bool_sector_empty && !bool_got_last_sector) {
            sector_last_addr = i;
        } else if(bool_sector_empty && sector_last_addr >= 0) {
            bool_got_last_sector = 1;
        }

        //count sector empty and full
        if(bool_sector_empty) {
            sector_empty_cnt++;
        } else {
            sector_full_cnt++;
        }
    }

    //check the valid of sectors
    i = sector_last_addr + 1; //point to next free sector
    i %= p_w25qxx->SectorCount;
    for(j = 0; j < sector_empty_cnt; j++) {
        i %= p_w25qxx->SectorCount;
        if(!W25qxx_IsEmptySector(i, 0, 0)) {
            printf("Invalid sectos, need to erase all\n");
            //erase all chip
            sector_empty_cnt = p_w25qxx->SectorCount;
            sector_full_cnt = 0;
            sector_last_addr = p_w25qxx->SectorCount - 1;
            sector_wr_addr = 0;
            byte_wr_addr = sector_wr_addr * p_w25qxx->SectorSize;
            sector_erase_addr = 0;
            block_erase_addr = 0;

            return -1;
        }
        i++;
    }
    
    sector_erase_addr = i % p_w25qxx->SectorCount; //i current point to first full sector after last free sector
    block_erase_addr = (sector_erase_addr * p_w25qxx->SectorSize) / p_w25qxx->BlockSize;
    sector_wr_addr = (sector_last_addr + 1) % p_w25qxx->SectorCount;
    byte_wr_addr = sector_wr_addr * p_w25qxx->SectorSize;

    return 0;
}

//count sector_free_cnt, just check first byte of sector
int get_sector_free_cnt() {
    int i, sector_free_num = 0;
    uint8_t value;

    for(i = 0; i < p_w25qxx->SectorCount; i++) {
        value = W25qxx_ReadByte_Quick(i * p_w25qxx->SectorSize);
        if(value == 0xFF) {
            sector_free_num++;
        }
    }

    return sector_free_num;
}

//check remain sector free and erase if sector_free_cnt < SECTOR_FREE_LIMIT
//  in a thread
int erase_cnt = 0, erase_cnt_resume = 0;
int log_do_erase() {
    static int bool_erasing = 0;

    osStatus_t status = osMutexAcquire(spiFlash_mtID, 10);
    if(status != osOK) {
        spiFlashMutexTimeoutCnt++;
        return;
    }

    //wait until Busy = 0
    W25qxx_WaitBusy(0);
    
    if(W25qxx_IsSus()) {
        //send resume
        erase_cnt_resume++;
        W25qxx_Resume();
        // HAL_Delay(50); //500ms
    } else {
        if(bool_erasing) {
            bool_erasing = 0;
            // sector_empty_cnt++;
            sector_erase_addr = (sector_erase_addr + 1) % p_w25qxx->SectorCount;
            block_erase_addr = (block_erase_addr + 1) % p_w25qxx->BlockCount;
            // sector_wr_addr = byte_wr_addr / p_w25qxx->SectorSize;
        }
        //get sector_empty_cnt
        // sector_empty_cnt = get_sector_free_cnt();

        //get sector_empty_cnt
        sector_empty_cnt = get_sector_free_cnt();

        if(sector_empty_cnt < SECTOR_FREE_LIMIT) {
            //erase sector_erase_addr
            // W25qxx_EraseSector_NoWait(sector_erase_addr);
            //erase block_erase_addr
            W25qxx_EraseBlock_NoWait(block_erase_addr);
            bool_erasing = 1;
            erase_cnt++;
//            W25qxx_WaitBusy(0);
             // HAL_Delay(100); //500ms
        }
    }

    osMutexRelease(spiFlash_mtID);
}

void pr_debug(const char *fmt, ...) {
    if(PRINT_LEVEL > DEBUG_LEVEL)
        return;

    log_to_flash(fmt);
}

void pr_warn(const char *fmt, ...) {
    if(PRINT_LEVEL > WARNING_LEVEL)
        return;

    log_to_flash(fmt);
}

void pr_err(const char *fmt, ...) {
    if(PRINT_LEVEL > ERROR_LEVEL)
        return;

    log_to_flash(fmt);
}
