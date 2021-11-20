///////////////////////////////////////////////////////////////////////////////
// Source file firmwareLoader.c
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Firmware loader
///////////////////////////////////////////////////////////////////////////////

#include "firmwareLoader.h"
#include "emb_flash.h"
#include "main.h"

#if defined (RMA_COMPILATION) || (AMA_COMPILATION)
#include "setup.h"
#include "fota.h"
void read_ext_eeprom(u32 address, u8 *data, u16 len);
void write_ext_eeprom(u32 address, u8 *data, u16 len);
void download_image_to_slave(u32 slave_id, u32 ee_address, uint32_t size);
#endif
  
static uint8_t fwData[FW_BUFF_SIZE];

  
static fwImageInfo_t fwInfo;  

uint32_t firware_ee_offset = 0;



#ifdef SHELF_MS_COMPILATION

static volatile uint32_t ivector[SHELF_IVECTOR_TABLE_SIZE / 4] @ SHELF_IVECTOR_ADDR;

/**************************************************
* Function name	: fwCopyIntTable
* Returns	:	
* Arg		: uint32_t size
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwPlaceAppItTable(void)
{
  
  uint32_t* vt_src = (uint32_t*)SHELF_FW_START_ADDR;
  uint32_t* vt_dst = (uint32_t*)ivector;
  
  for(int i = 0; i <= SHELF_IVECTOR_TABLE_SIZE; i++)
  {
    *(vt_dst + i) = *(vt_src + i);
  }
  //memcpy (((uint8_t*)SHELF_FW_START_ADDR), *(uint8_t*)SHELF_IVECTOR_ADDR, SHELF_IVECTOR_TABLE_SIZE);
}

#endif
/**************************************************
* Function name	: fwInitSpace
* Returns	:	
* Arg		: uint32_t size
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwInitSpace(uint32_t size)
{
  memset(fwData, 0, sizeof(fwData)); 
  __NOP();
}


/**************************************************
* Function name	: fwInitImage
* Returns	:	
* Arg		: uint8_t* header
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwInitImage(uint8_t* header)
{
  
  uint32_t fileHeader[4];
  uint8_t offset = 0;

  fwInfo.PackCnt = 0;
  //Get the image header from the packet
  memcpy(fileHeader, header + offset, sizeof(fileHeader));
  
  fwInfo.Length        = fileHeader[0];
  fwInfo.Checksum      = fileHeader[1];
  fwInfo.fwVersion     = fileHeader[2];
  fwInfo.ChecksumMatch = 0;
  fwInfo.devType       = (CARD_TYPE)((fwInfo.fwVersion & 0x00ff0000) >> 16);
  fwInfo.fwData        = fwData;
  
  switch(fwInfo.devType)
  {
  case CTYPE_AMA    : 
#ifndef BOOTLOADER_COMPILATION
    fwDeinitImage();
    RestartMCU();
#endif    
    break;
  case CTYPE_RMA    : 
    firware_ee_offset = EE_RMA_ADDR;
    break;
  case CTYPE_SHELF  : 
    firware_ee_offset = EE_SHELF_ADDR;
    break;
  case CTYPE_CAMERA : 
    firware_ee_offset = EE_CAMERA_ADDR;
    break;
  }
  
  if(fwInfo.Checksum == fileHeader[3])
  {
    fwInfo.Length = fileHeader[0];
    fwInfo.Checksum = fileHeader[1];
  }
  
}

/**************************************************
* Function name	: fwLoaderCollectData
* Returns	:	
* Arg		: uint8_t* data, uint16_t len, uint16_t idx
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwLoaderCollectData(uint8_t* data, uint16_t len, uint16_t idx)
{
  uint16_t pcktLen = len - PROTOCOL_OVERHEAD;
  fwInfo.CurrPackIdx = idx;
  static uint32_t ee_addr = 0;
  
#if  defined  (BOOTLOADER_COMPILATION)
  
  memcpy(fwData + (idx * pcktLen), data + PROTOCOL_DATA_OFFSET, pcktLen);
  
#elif defined (AMA_COMPILATION)
  
  if(fwInfo.devType != CTYPE_AMA)
  {
    ee_addr = firware_ee_offset + (idx * (len - PROTOCOL_OVERHEAD));
    write_ext_eeprom(ee_addr, data + PROTOCOL_DATA_OFFSET, pcktLen);
  }
  
#elif  defined (RMA_COMPILATION)
  
  ee_addr = firware_ee_offset + (idx * len);
  write_ext_eeprom(ee_addr, data, len);
  
#endif
}

void startSendFw(void)
{
  firware_ee_offset = EE_RMA_ADDR;
  
  fwValidateImage();
}
/**************************************************
* Function name	: fwValidateCrc
* Returns	:	
* Arg		: uint8_t* pckt, uint16_t len
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
uint8_t fwValidateCrc(uint8_t* pPckt, uint16_t len)
{
  uint16_t crc = 0;
  //Get the CRC from the tail of the packet
  memcpy(&crc, (uint8_t*)pPckt + len - 2, 2);
    
  return ( crc == ProtocolCalcCrc((const uint8_t*) pPckt ,len - 2) );
}

/**************************************************
* Function name	: fwCalcImageChecksum
* Returns	:	
* Arg		: uint8_t* fw, uint32_t len
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
uint32_t fwCalcImageChecksum(uint8_t* fw, uint32_t len)
{
  uint32_t checkSum = 0;
  static uint32_t i = 0;
  
  for (i = FW_HEADER_SIZE; i < len + FW_HEADER_SIZE; i++)
  {
      checkSum += (uint32_t)(fw[i] & 0xFF);
  }
    
  return checkSum;
}


uint32_t fwCalcEepromImageChecksum(uint32_t addr, uint32_t len)
{
  uint32_t checkSum = 0;
  uint8_t tmp;
  
  for (uint32_t i = FW_HEADER_SIZE; i < len + FW_HEADER_SIZE; i++)
  {
    read_ext_eeprom(i, &tmp, 1);
    
    checkSum += (uint32_t)(tmp & 0xFF);
  }
    
  return checkSum;
}
/**************************************************
* Function name	: fwValidateImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
int fwValidateImage(void)
{
  uint8_t tempBuff[FW_HEADER_SIZE];
  uint32_t checkSum = 0;
  uint8_t ret = 0;
#if defined (RMA_COMPILATION) || (AMA_COMPILATION)
  
  switch(fwInfo.devType)
  {
  case CTYPE_RMA    : 
    firware_ee_offset = EE_RMA_ADDR;
    break;
  case CTYPE_SHELF  : 
    firware_ee_offset = EE_SHELF_ADDR;
    break;
  case CTYPE_CAMERA : 
    firware_ee_offset = EE_CAMERA_ADDR;
    break;
  }
  
  read_ext_eeprom(firware_ee_offset, tempBuff, sizeof(tempBuff));
  
  fwInitImage(tempBuff);
  
 // if(fwInfo.Length <= sizeof(fwData))
  {
    checkSum = fwCalcEepromImageChecksum(firware_ee_offset,fwInfo.Length);
    //read_ext_eeprom(firware_ee_offset + RMA_FIRMWARE_EEPROM_OFFSET, fwData, fwInfo.Length + FW_HEADER_SIZE);
  }
#elif defined (BOOTLOADER_COMPILATION)  
  checkSum = fwCalcImageChecksum(fwData, fwInfo.Length);
#endif
    
  
    
    if(checkSum && checkSum == fwInfo.Checksum)
    {
    ret = 1;
      fwInfo.ChecksumMatch = 1;
#if defined (BOOTLOADER_COMPILATION)      
      fwWriteToFlash();
#elif defined (AMA_COMPILATION)
      
      //Pass to RMA
      fotaStartTransmitFw(&fwInfo);
#elif defined (RMA_COMPILATION)
      
      switch(fwInfo.devType)
      {
      case CTYPE_RMA    : 
        fwDeinitImage();
        fwDeinitImage();
        RestartMCU();
        break;
      case CTYPE_SHELF  : 
        //Pass to shelves
      send_code_image_to_slaves(firware_ee_offset, fwInfo.Length + FW_HEADER_SIZE);
//        download_image_to_slave(0x102142, firware_ee_offset, fwInfo.Length + FW_HEADER_SIZE);
        __NOP();
        break;
      case CTYPE_CAMERA : 
        break;
      }
      
      //Pass to slaves
      //For Noam
#endif      
    }else
    {
      fwInfo.ChecksumMatch = 0;

    }
    return ret;
}

/**************************************************
* Function name	: fwValidateFlashImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
uint8_t fwValidateFlashImage(void) 
{
  uint32_t fwLength;
  uint32_t checkSum;
  uint8_t tempBuff[FW_HEADER_SIZE];
    
  memcpy(&fwLength, (uint8_t*)FW_PLACE_ADDR, 4);
  memcpy(&checkSum, (uint8_t*)FW_PLACE_ADDR + 4, 4);
  

  if(fwLength && fwLength < FW_MAX_APP_SIZE)
  {

    if(checkSum == fwCalcImageChecksum((uint8_t*)FW_PLACE_ADDR, fwLength))
    {
      //Jump to application
      fwStartApp();
    }else
    {
#if defined (AMA_COMPILATION) || (BOOTLOADER_COMPILATION) || (RMA_COMPILATION)
      //Erase 300K bytes
      fwFlashPrepare();
      
#ifdef RMA_BOOTLOADER
      
      RestartMCU();
      
#endif
#else
      
      fwCopyFromFlashToFlash(SHELF_FW_PLACE_ADDR, SHELF_FW_TEMP_SAVE_ADDR, fwLength + FW_HEADER_SIZE);
      
#endif
    }
  }else
  {
#if defined (AMA_BOOTLOADER)    
    //Erase 300K bytes
    fwFlashPrepare();
    
#elif defined (RMA_BOOTLOADER)    

    //checksum check
    
      read_ext_eeprom(firware_ee_offset, tempBuff, sizeof(tempBuff));
  
  fwInitImage(tempBuff);
  
  if(fwInfo.Length <= sizeof(fwData))
  {
    checkSum = fwCalcEepromImageChecksum(firware_ee_offset,fwInfo.Length);
    //read_ext_eeprom(firware_ee_offset + RMA_FIRMWARE_EEPROM_OFFSET, fwData, fwInfo.Length + FW_HEADER_SIZE);
  }
    
    if(checkSum && checkSum == fwInfo.Checksum)
    {
      __NOP();
    //Copy to flash
      
      //Erase 
      fwFlashPrepare();
      fwCopyFromEepromToFlash();
    
      RestartMCU();
    }
#else
  fwCopyFromFlashToFlash(SHELF_FW_PLACE_ADDR, SHELF_FW_TEMP_SAVE_ADDR, fwLength + FW_HEADER_SIZE);
#endif    
    
  }
  return 0;
}


/**************************************************
* Function name	: fwValidateImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwCopyFromFlashToFlash(uint32_t dstAddr, uint32_t srcAddr, uint32_t size)
{
  
  embFlashEraseSector(7,29);
    
  fwInitImage((uint8_t*)srcAddr);
  
  delay_ms(500);
  
  for (uint32_t i = 0; i < (fwInfo.Length + FW_HEADER_SIZE)/4; i++)
  {
      
    embFlashWrite(dstAddr , *(uint64_t*)srcAddr );
    
    dstAddr += 4;
    srcAddr += 4;
    
    //delay_ms(1);
  }
  
  HAL_FLASH_Lock();
  
  fwValidateFlashImage();
}

/**************************************************
* Function name	: fwValidateImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwCopyFromEepromToFlash(void)
{
  
  uint64_t word = 0;
  //uint8_t tempBuff[4];
    
  delay_ms(500);
  
  for (uint32_t i = 0; i < (fwInfo.Length + FW_HEADER_SIZE)/4; i++)
  {
     
    read_ext_eeprom(firware_ee_offset + (i * 4), (uint8_t*)&word, sizeof(word));
     
    //memcpy(&word, tempBuff, 4);
    
    embFlashWrite(FW_PLACE_ADDR + (i * 4), word);
  }
  
  HAL_FLASH_Lock();
  
  //fwValidateFlashImage();
}

/**************************************************
* Function name	: fwValidateImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwWriteToFlash(void)
{
  
  uint64_t word = 0;
  HAL_Delay(500);
  
  for (uint32_t i = 0; i < (fwInfo.Length + FW_HEADER_SIZE)/4; i++)
  {
    memcpy(&word, fwData + (i * 4), 4);
    
    embFlashWrite(FW_PLACE_ADDR + (i * 4), word);
  }
  
  HAL_FLASH_Lock();
  
  fwValidateFlashImage();
}

/**************************************************
* Function name	: fwStartApp
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
typedef void (*pFunction)(void);
pFunction app_main_addr;

void fwStartApp(void)
{
uint32_t addr;

#ifdef BOOTLOADER_COMPILATION  
   
  __IO uint32_t*  pAdd = (__IO uint32_t *)(FW_START_ADDR + 4); //pointer to reset handler of application 
  SCB->VTOR = FW_START_ADDR;           // VTOR: set the vector table to that of the app 
  __set_MSP(*(__IO uint32_t*)FW_START_ADDR);             //set the stack pointer to that of the application 
  
  ((void (*)())(*pAdd))(); // pointer recast as function pointer and the dereferenced/called
  while (1){ };  
#else
  
   fwPlaceAppItTable();

    addr = *(uint32_t *)(FW_START_ADDR);              // get application SP
  __set_MSP(addr);
  addr = *(uint32_t *)(FW_START_ADDR + 4);
  app_main_addr = (pFunction)addr;             // get address of app main
  app_main_addr();
  
//  __IO uint32_t*  pAdd = (__IO uint32_t *)(FW_START_ADDR + 4); //pointer to reset handler of application 
//  __set_MSP(*(__IO uint32_t*)FW_START_ADDR);             //set the stack pointer to that of the application   
//  ((void (*)())(*pAdd))(); // pointer recast as function pointer and the dereferenced/called
  while (1); 
  
#endif
}

/**************************************************
* Function name	: fwDeinitImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwDeinitImage(void)
{
#ifndef STM32F030_COMPILATION   
  embFlashEraseSector(4,1);
#else

  embFlashEraseSector(7,1);
#endif  
}

/**************************************************
* Function name	: fwDeinitImage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: 
**************************************************/
void fwFlashPrepare(void)
{
  uint8_t erase = 0;
  
  //Skipp erasing clear flash
  for(uint32_t i = FW_PLACE_ADDR; i < (FW_PLACE_ADDR + FW_MAX_APP_SIZE); i++)
  {
    if (*(uint8_t*)i != 0xFF )
    {
      erase = 1;
      break;
    }
  }

  if(erase)
  {
    embFlashEraseSector(4,2);
  }
}

/**************************************************
* Function name	: getFwInfo()
* Returns	: fwImageInfo_t*
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 08/02/2020
* Description	: 
* Notes		: Returns the pointer to the Fwinfo
**************************************************/
fwImageInfo_t* getFwInfo(void)
{
  return &fwInfo;
}