///////////////////////////////////////////////////////////////////////////////
// Source file firmwareLoader.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Firmware loader header file
///////////////////////////////////////////////////////////////////////////////

#ifndef __FIRMWARE_LOADER_H
#define __FIRMWARE_LOADER_H

#include "main.h"
#include "protocol.h"
#include <string.h>

#define PROTOCOL_DATA_OFFSET 7
#define PROTOCOL_OVERHEAD 9

#define FW_HEADER_SIZE 16

#if defined(BOOTLOADER_COMPILATION)
#define FW_START_ADDR  0x08020000
#define FW_BUFF_SIZE (1024 * 64)

#elif defined(AMA_COMPILATION) || defined(RMA_COMPILATION)
#define FW_START_ADDR  0x08020000
#define FW_BUFF_SIZE (1024 * 1)

#elif defined(STM32F030_COMPILATION)
#define FW_START_ADDR  0x08002000

#define FW_BUFF_SIZE (256)
#endif

#define FW_PLACE_ADDR  (FW_START_ADDR - FW_HEADER_SIZE)

#define FW_MAX_APP_SIZE (1024 * 64)

//Boot loader 0x08000000 -> + 7K
//Application header0x08001C00
//Application code 0x08002000 -> + 28K
//Temporary image 0x08009000 -> + 28K
#define SHELF_FW_TEMP_SAVE_ADDR   0x08009000
#define SHELF_FW_START_ADDR       0x08002000
#define SHELF_FW_PLACE_ADDR       SHELF_FW_START_ADDR - 16
#define SHELF_IVECTOR_ADDR        0x20000000
#define SHELF_IVECTOR_TABLE_SIZE  0xC0


#define FW_END_ADDR  (FW_START_ADDR + FW_MAX_APP_SIZE)

#define RMA_FIRMWARE_EEPROM_OFFSET 0

typedef enum {
  CTYPE_AMA    = 0,
  CTYPE_RMA    = 1,
  CTYPE_SHELF  = 2,
  CTYPE_CAMERA = 3
} CARD_TYPE;

#define EE_RMA_ADDR     0x00000
#define EE_SHELF_ADDR   0x00000
#define EE_CAMERA_ADDR  0x18000
#define EE_FLAGS_ADDR   0x1FFF0


typedef struct  
{
  CARD_TYPE devType;
  uint8_t  ChecksumMatch;
  uint8_t* fwData;
  uint32_t Length;
  uint32_t Checksum;
  uint32_t fwVersion;
  uint16_t CurrPackIdx;
  uint16_t PrevPackIdx;
  uint16_t PacksInFile;
  uint16_t PackCnt;
} fwImageInfo_t;

void fwLoaderCollectData(uint8_t* data, uint16_t len, uint16_t idx);
void fwInitSpace(uint32_t size);

uint8_t* getPhysFrameBufferPtr(void);
uint8_t fwValidateCrc(uint8_t* pckt, uint16_t len);
uint8_t fwValidateFlashImage(void);
void fwInitImage(uint8_t* header);
void fwDeinitImage(void);
int fwValidateImage(void);
void fwWriteToFlash(void);
void fwStartApp(void);
void fwFlashPrepare(void);
fwImageInfo_t* getFwInfo(void);
void fwCopyFromEepromToFlash(void);
void fwCopyFromFlashToFlash(uint32_t dstAddr, uint32_t srcAddr, uint32_t size);
#endif //__FIRMWARE_LOADER_H