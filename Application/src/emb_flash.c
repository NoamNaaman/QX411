///////////////////////////////////////////////////////////////////////////////
// Source file emb_flash.c
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Ebmedded flash R/W methods
///////////////////////////////////////////////////////////////////////////////

#include "emb_flash.h"
#include "string.h"



/**************************************************
* Function name	: embFlashEraseSector
* Returns	:	
* Arg		: uint8_t sector
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: 
* Notes		: 
**************************************************/
void embFlashEraseSector(uint8_t sector, uint8_t numOfSec)
{
  uint32_t PageError;
  FLASH_EraseInitTypeDef EraseInit;
  
    
#if defined (AMA_COMPILATION) || (BOOTLOADER_COMPILATION) || (RMA_COMPILATION)
  EraseInit.Banks = FLASH_BANK_1;
  EraseInit.Sector = sector;
  EraseInit.NbSectors = numOfSec;
  EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#else
  
  EraseInit.PageAddress = ( 0x08000000 + ( sector * 1024));
  EraseInit.NbPages = numOfSec;
  
#endif  
    HAL_FLASH_Unlock();
    delay_ms(100);
  //Reset Option bits verification error bit 
 // FLASH->SR |= FLASH_SR_EOP;
  
  HAL_FLASHEx_Erase(&EraseInit, &PageError);
    
  HAL_FLASH_Lock();

}


/**************************************************
* Function name	: embFlashWrite()
* Returns	:	
* Arg		: uint32_t addr, uint64_t data
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: 
* Notes		: 
**************************************************/
void embFlashWrite(uint32_t addr, uint64_t data)
{
    
  HAL_FLASH_Unlock();
  //Reset Option bits verification error bit 
  FLASH->SR |= FLASH_SR_EOP;
  
  while(FLASH->SR & FLASH_SR_BSY);
  
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data);
  
  //HAL_FLASH_Lock();
  
}