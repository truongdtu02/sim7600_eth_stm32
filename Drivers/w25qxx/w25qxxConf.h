#ifndef _W25QXXCONFIG_H
#define _W25QXXCONFIG_H

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#define _W25QXX_SPI                   hspi1
#define _W25QXX_CS_GPIO               FLASH_CS_GPIO_Port
#define _W25QXX_CS_PIN                FLASH_CS_Pin
#define _W25QXX_USE_FREERTOS          0
#define _W25QXX_DEBUG                 0

#endif
