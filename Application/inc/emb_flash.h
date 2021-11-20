///////////////////////////////////////////////////////////////////////////////
// Source file emb_flash.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Ebmedded flash R/W methods
///////////////////////////////////////////////////////////////////////////////

#ifndef __EMB_FLASH_H

#include "main.h"

#define FLASH_BASE_ADDR  0x08000000


void embFlashLock(void);
void embFlashUnlock(void);
void embFlashWrite(uint32_t addr, uint64_t data);
void embFlashEraseSector(uint8_t sector, uint8_t numOfSec);

#define __EMB_FLASH_H

#endif //__EMB_FLASH_H