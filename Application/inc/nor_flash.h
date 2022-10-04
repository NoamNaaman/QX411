/**
******************************************************************************
* File Name          : nor_flash.h
* Date               : 01/9/2019 
* Description        : NOR Flash driver
* Author	     : Lev Zoosmanovskiy
******************************************************************************

******************************************************************************
*/

#ifndef __NOR_FLASH_H
#define __NOR_FLASH_H

#include "main.h"
#include "nor_flash.h"

//-- external serial nor flash definition --//
#define FLASH_SPI                     SPI1

#define FLASH_SPI_SERVICE_INFO        (4)
#define FLASH_SPI_4SERVICE_INFO       (5)
#define FLASH_SPI_CMND_SERVICE_INFO   (1)

#define FLASH_SPI_OFFSET_CMND         (0)
#define FLASH_SPI_OFFSET_ADD2         (1) // MSB
#define FLASH_SPI_OFFSET_ADD1         (2)
#define FLASH_SPI_OFFSET_ADD0         (3)
#define FLASH_SPI_OFFSET_DATA         (4)

//#define FLASH_SPI_4OFFSET_ADD3        (1) // MSB
//#define FLASH_SPI_4OFFSET_ADD2        (2) 
//#define FLASH_SPI_4OFFSET_ADD1        (3)
//#define FLASH_SPI_4OFFSET_ADD0        (4)
//#define FLASH_SPI_4OFFSET_DATA        (5)
#define M95M01_ADDR_TOP        (1) 
#define M95M01_ADDR_MID        (2)
#define M95M01_ADDR_BOT        (3)

//-- 4 bytes address commands --//
#define FLASH_4WRITE_PAGE_CMND        (0x12)
#define FLASH_4READ_DATA_CMND         (0x13)
#define FLASH_4ERASE_SUBSCT4K_CMND    (0x21) // 50mS
#define FLASH_4ERASE_SUBSCT32K_CMND   (0x5C) // 100mS
#define FLASH_4ERASE_SCT_CMND         (0xDC) // 150mS
#define FLASH_4ERASE_BULK_CMND        (0x60) // 150mS

//-- 3 bytes address commands --//
#define FLASH_READ_CHIP_ID            (0x83) // (0x9F) for Winbond
#define FLASH_WRITE_CHIP_ID           (0x82)
#define FLASH_WRITE_ST_CMND           (0x01)
#define FLASH_READ_ST_CMND            (0x05)
#define FLASH_READ_FLST_CMND          (0x70)
#define FLASH_ERASE_DIE_CMND          (0xC4)
#define FLASH_ERASE_SUBSCT_CMND       (0x20)
#define FLASH_ERASE_SCT_CMND          (0xD8)
#define FLASH_WRITE_PAGE_CMND         (0x02)
#define FLASH_READ_DATA_CMND          (0x03)
#define FLASH_WRITE_ENB_CMND          (0x06)
#define FLASH_WRITE_DIS_CMND          (0x04)

//#define FLASH_ENTER_4BYTE_ADD         (0xB7)
//#define FLASH_EXIT_4BYTE_ADD          (0xE9)
//#define FLASH_READ_NVOL_CNFG          (0xB5)
//#define FLASH_WRITE_NVOL_CNFG         (0xB1)
//#define FLASH_READ_VOL_CNFG           (0x85)
//#define FLASH_WRITE_VOL_CNFG          (0x81)
#define FLASH_ENTER_DEEP_SLEEP        (0xB9)
#define FLASH_RELEASE_FROM_DEEP       (0xAB)

#define SZ_STREAM_BLOCK               (0x80)
#define SZ_STREAM_PKT                 (0x80)
#define SZ_NOR_FLASH_PAGE             (0x100)
#define SZ_NOR_FLASH_SUB_SCT          (0x1000)

#define NOR_FLASH_DEFAULT_VALUE       (0xFF)

#define NOR_FLASH_ST_BIT_WRBUSY       (0x01)
#define NOR_FLASH_ST_BIT_WRENB        (0x02)


extern volatile bool g_bSemSpiOneBusy;

void  FlashReadChipId(u8* pB);
void  NorFlashWriteRegCmnd(u8 u8Cmnd);
void init_nor_flash(void);

#endif //__NOR_FLASH_H