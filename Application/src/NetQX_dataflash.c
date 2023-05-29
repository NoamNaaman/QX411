///////////////////////////////////////////////////////////////////////////////
// Source file dataflash.c
//
// Written by: Noam Naaman
// email:   n.noam@yahoo.com
// Copyright © 2020, Noam Naaman
//
///////////////////////////////////////////////////////////////////////////////

#include "stm32F4xx_hal.h"
#include "string.h"
#include "setup.h"
#include "main.h"
#include "serial.h"
#include "nor_flash.h"


#define FLUSH_DELAY_CYCLES 4

#define NOR_PAGE_SIZE 256
#define NOR_FLASH_SIZE 0x4000000
#define EXT_FLASH_LAST_BLOCK (NOR_FLASH_SIZE - 0x10000)

static volatile u8 g_u8FlashPrefetchBuffer[SZ_NOR_FLASH_PAGE];
u16 g_u16NorFlashRegState;

volatile u8 *g_pFlashReadDestPtr;

u32 ee_type;

u32 base_doors_setup = 0x8000;

u32 g_u32PageDataIndex;
u32 eeprom_write_timer;

u32 g_u32FlshDataUploadInProgressCtr, g_u32FlashPrefetches;
u32 g_u32ExtraBits, g_u32LatestSuspectPeak, g_u32LatestVerifiedPeak;

u32 gt_u32PpgInput[60];

bool g_bFlashOperatingOK;


u32  g_u32FlashReadAddr, FlashChipSelect;

extern u8 *g_pSpi1DataDest;

u8            g_u8SpiOneRxBuff[NOR_PAGE_SIZE*2];
u8            g_u8SpiOneTxBuff[NOR_PAGE_SIZE];
volatile u32 g_u32SpiOneTxLen;
volatile u32 g_u32SpiOneRxLen;
volatile u32 g_u32SpiOneRxCount;
volatile u32 g_u32SpiOneTxCount;

extern SPI_HandleTypeDef hspi1;

u32  FlashReadStatusReg(void);
u8  NorFlashReadRegCmnd(u8 u8Cmnd);
u32  FlashReadFlagStatusReg(void);
void FlashChipEnable(void);
void FlashChipDisable(void); 
void delay_ms(u32 ms);


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void DelayCycles(u32 cycles)
  {
  while (cycles--);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void init_ports(void)
    {
    output_drive(EECS1);
    output_drive(EECS2);
    output_drive(EECS3);
    output_drive(EECS4);
    output_high(EECS1);
    output_high(EECS2);
    output_high(EECS3);
    output_high(EECS4);
    }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void SelectFlashCS(u32 address)
  {
  init_ports();
  if (ee_type == 1)
    {
    address >>= 17; // get 2 device select bits
    FlashChipSelect = address & 3;
    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashBlockingXfr(u8 *TxBuff, u8 *RxBuff, u32 TxLen, u32 RxLen)
  {
  u8 data, byte;
  FlashChipEnable();
//  delay_us(1);

  HAL_SPI_Transmit(&hspi1, TxBuff, TxLen, TxLen);
//  while (TxLen--)
//    {
//    byte = *TxBuff++;
//    SPI1->DR = byte;
//    while ((SPI1->SR & SPI_SR_TXE) == 0);
//    delay_us(1);
//    data = SPI1->DR;
//    }
  if (RxLen)
    {
//    HAL_SPI_TransmitReceive(&hspi1, TxBuff, RxBuff, RxLen, RxLen);
//    }
//  while (RxLen)
//    {
//    SPI1->DR = 0xFD;
    HAL_SPI_TransmitReceive(&hspi1, TxBuff, RxBuff, RxLen, 100);
//    while ((SPI1->SR & SPI_SR_TXE) == 0);
//    delay_us(1);
//    *RxBuff++ = SPI1->DR;
//    RxLen--;
    }
  delay_us(1);
  FlashChipDisable();
  }


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashWriteStatusRegister(u8 data)
  {
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_WRITE_ST_CMND;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND+1] = data;
  g_u32SpiOneTxLen = 2;
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, 0); 
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  FlashSctErase(u32 u32Add)
  {
//  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
  SelectFlashCS(u32Add);
  u32Add &= 0x1FFFF; // adust address to 128K per chip

  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
  
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_ERASE_SCT_CMND;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
  g_u32SpiOneTxLen = FLASH_SPI_4SERVICE_INFO;
  g_u32SpiOneRxLen = 0;
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  FlashEraseSector(u32 u32Add)
  {
  static u32 stat;
  SelectFlashCS(u32Add);
  u32Add &= 0x1FFFF; // adust address to 128K per chip

  stat = FlashReadStatusReg();
//  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
  
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_ERASE_SCT_CMND;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
  g_u32SpiOneTxLen = FLASH_SPI_4SERVICE_INFO;
  g_u32SpiOneRxLen = 0;

  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  stat = FlashReadStatusReg();
  stat = FlashReadStatusReg();
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashWriteData(u32 u32Add, u32 u32Len, u8* pBuff)
  {
  u32 len = 0;
//  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
  output_drive(SF_WP);
  output_high(SF_WP);
  SelectFlashCS(u32Add);
//  u32Add &= 0x1FFFF; // adjust address to 128K per chip
  
  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
  
  g_u8SpiOneTxBuff[len++] = FLASH_WRITE_PAGE_CMND;
//  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 24);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 0);
//  g_u8SpiOneTxBuff[len++] = 0;
  memcpy(g_u8SpiOneTxBuff + len, pBuff, u32Len);
  g_u32SpiOneTxLen = len + u32Len;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, 0); 
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashReadData(u32 u32Add, u32 u32Len, u8 *DestBuffer)
  {
  SelectFlashCS(u32Add);
//  u32Add &= 0x1FFFF; // adust address to 128K per chip FOR 1M CHIPS

  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_READ_DATA_CMND;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
//  g_u8SpiOneTxBuff[FLASH_SPI_4OFFSET_DATA] = 0;
  g_u32SpiOneTxLen = FLASH_SPI_SERVICE_INFO;
  g_u32SpiOneRxLen = u32Len;
  
//  g_pFlashReadDestPtr = DestBuffer;

  FlashBlockingXfr(g_u8SpiOneTxBuff, DestBuffer, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  
//  memcpy(DestBuffer, g_u8SpiOneRxBuff, u32Len);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashEraseData(void)
  {
  u32 faddress = 0, count, not_ff;
  u8 buf[256];
//  PC_send_string("\r\nErasing flash data...\r\n");
  while (faddress < NOR_FLASH_SIZE - 0x10000)
    {
    FlashReadData(faddress, 256, buf);
    for (not_ff = 0, count = 0; count < 256; count++)
      {
      if (buf[count] != 0xFF)
        {
        not_ff++;
        }
      }
    if (not_ff == 0)
      {
      break;
      }
    FlashEraseSector(faddress);
    PC_send_string(".");
    delay_ms(1000);
    faddress += 0x10000;
    }
  FlashEraseSector(NOR_FLASH_SIZE - 0x10000);
//  PC_send_string("\r\nDone\r\n");
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FetchUploadPage(u32 addr)
  {
  g_u32FlashReadAddr = addr;
  FlashReadData(g_u32FlashReadAddr, SZ_NOR_FLASH_PAGE, (u8 *)g_u8FlashPrefetchBuffer);
  g_pSpi1DataDest = (u8 *)&g_u8FlashPrefetchBuffer[0];
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  NorFlashWriteRegCmnd(u8 u8Cmnd)
  {
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = u8Cmnd;
  
  g_u32SpiOneTxLen = FLASH_SPI_CMND_SERVICE_INFO;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, NULL, g_u32SpiOneTxLen, 0);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
u8  NorFlashReadRegCmnd(u8 u8Cmnd)
  {
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = u8Cmnd;;
  
  g_u32SpiOneTxLen = FLASH_SPI_CMND_SERVICE_INFO;
  g_u32SpiOneRxLen = 1;
  
  g_pFlashReadDestPtr = g_u8SpiOneRxBuff;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen);
  
  return *g_u8SpiOneRxBuff;
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
u32  FlashReadStatusReg(void)
  {
  u32 status;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = FLASH_READ_ST_CMND;
  
  g_u32SpiOneTxLen = FLASH_SPI_CMND_SERVICE_INFO;
  g_u32SpiOneRxLen = 4;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  
  status = g_u8SpiOneRxBuff[0];//(u16)g_u8SpiOneRxBuff[0] | ((u16)g_u8SpiOneRxBuff[1] << 8);
  return status;
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  FlashReadChipId(u8* pB)
  {
  u32 u32Add = 0;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = FLASH_READ_CHIP_ID;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
  g_u32SpiOneTxLen = FLASH_SPI_SERVICE_INFO;
  g_u32SpiOneRxLen = 8;
  
  g_pFlashReadDestPtr = pB;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen);

  memcpy(pB, g_u8SpiOneRxBuff, 3);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashWriteChipId(u32 u32Add, u32 u32Len, u8* pBuff)
  {
  u32 len = 0;
//  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
  
  g_u8SpiOneTxBuff[len++] = FLASH_WRITE_CHIP_ID;
//  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 24);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[len++] = (u8)(u32Add >> 0);
//  g_u8SpiOneTxBuff[len++] = 0;
  memcpy(g_u8SpiOneTxBuff + len, pBuff, u32Len);
  g_u32SpiOneTxLen = len + u32Len;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, 0); 
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
u32  FlashReadFlagStatusReg(void)
  {
  u32 status;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = FLASH_READ_FLST_CMND;
  
  g_u32SpiOneTxLen = FLASH_SPI_CMND_SERVICE_INFO;
  g_u32SpiOneRxLen = 2;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  
  status = (u16)g_u8SpiOneRxBuff[0] | ((u16)g_u8SpiOneRxBuff[1] << 8);
  return status;
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  FlashChipCompleteErase(void)
  {
//  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
  
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND] = FLASH_ERASE_DIE_CMND;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND+1] = 0;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND+2] = 0;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND+3] = 0;
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND+4] = 0;

  g_u32SpiOneTxLen = FLASH_SPI_4SERVICE_INFO;
  
  g_u32SpiOneRxLen = 0;
  
  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  }


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
bool FlashPageIsNotErased(u32 u32Add)
  {
  FlashReadData(u32Add, SZ_NOR_FLASH_PAGE, g_u8SpiOneRxBuff); // ~450uS on /4/2 freq
  for (u32 u32I = 0; u32I < SZ_NOR_FLASH_PAGE; u32I++)
    {
    if (g_u8SpiOneRxBuff[u32I] != 0xFF)
      {
      return true;
      }
    }
  return false; 
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  InitExtFlash(void)
  {
  static u8 pBuff[4] = { 0,0,0 };

  output_drive(FLASH_CS1);
  output_drive(FLASH_PROTECT);
  output_drive(FLASH_RESET);
  output_drive(EECS1);
  output_drive(EECS2);
  output_drive(EECS3);
  output_drive(EECS4);
  
  output_high(EECS1);
  output_high(EECS2);
  output_high(EECS3);
  output_high(EECS4);
  output_high(FLASH_CS1);
  output_high(FLASH_PROTECT);
  output_high(FLASH_RESET);
  delay_us(10);
  output_low(FLASH_RESET);
  delay_us(10);
  output_high(FLASH_RESET);

  FlashChipDisable();
  
  g_bFlashOperatingOK = true;

  for (u32 chip = 0; chip < 4; chip++)
    {
    FlashChipSelect = chip;
    FlashReadChipId(pBuff);
    NorFlashWriteRegCmnd(FLASH_RELEASE_FROM_DEEP);
    }
  
//  //-- get out from deep sleep --//
//  NorFlashWriteRegCmnd(FLASH_RELEASE_FROM_DEEP);
//  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
//  g_u16NorFlashRegState = NorFlashReadRegCmnd(FLASH_READ_ST_CMND);
  
//  g_u16NorFlashRegState = NorFlashReadRegCmnd(FLASH_READ_NVOL_CNFG);
//  g_u16NorFlashRegState = NorFlashReadRegCmnd(FLASH_READ_VOL_CNFG);
  g_u16NorFlashRegState = NorFlashReadRegCmnd(FLASH_READ_ST_CMND);
//  if (g_u16NorFlashRegState & NOR_FLASH_ST_BIT_WRENB)
//    {
//    NorFlashWriteRegCmnd(FLASH_WRITE_DIS_CMND);
//    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//void  FlashSubSctErase(u32 u32Add)
//  {
////  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
//  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
//  
//  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_ERASE_SUBSCT4K_CMND;
////  g_u8SpiOneTxBuff[FLASH_SPI_4OFFSET_ADD3] = (u8)(u32Add >> 24);
//  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
//  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
//  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
//  g_u32SpiOneTxLen = FLASH_SPI_4SERVICE_INFO;
//  
//  FlashBlockingXfr(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, 0); 
//  }
//
/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//u32 FlashBulkErase(void)
//  {
//  u32 count = 0;
////  NorFlashWriteRegCmnd(FLASH_ENTER_4BYTE_ADD);
//  NorFlashWriteRegCmnd(FLASH_WRITE_ENB_CMND);
//  NorFlashWriteRegCmnd(FLASH_ERASE_BULK_CMND);
//  while (1)
//    {
//    FlashReadStatusReg();
//    if ((g_u8SpiOneRxBuff[0] & 1) == 0)
//      {
//      break;
//      }
//    count++;
//    }
//  
//  return count;
//  }


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//u32 FlashWriteTest(void)
//  {
//  u8 wbuf[256], rbuf[256];
//  u32 index, value;  
//
//  FlashBulkErase();
//  delay_ms(1000);
//  FlashReadData(EXT_FLASH_LAST_BLOCK, 256, rbuf);
//  for (index = 0; index < 128; index++)
//    {
//    wbuf[index] = (index) & 0xFF;
//    }
//  delay_us(100000);
//  FlashWritePage(EXT_FLASH_LAST_BLOCK, 128, wbuf);
//  delay_us(200000);
//  memset(rbuf, 0, 256);
//  FlashReadData(EXT_FLASH_LAST_BLOCK, 128, rbuf);
//  delay_us(200000);
//  for (index = 0, value = 0; index < 128; index++)
//    {
//    if (wbuf[index] != g_u8SpiOneRxBuff[index])
//      {
//      value++;
//      }
//    }
//  return value == 0;
//  }
//

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashChipEnable(void)
  {
  if (ee_type == 4)
    {
    output_low(EECS1);
    }
  else
    {
    switch (FlashChipSelect)
      {
      case 0: output_low(EECS1); break;
      case 1: output_low(EECS2); break;
      case 2: output_low(EECS3); break;
      case 3: output_low(EECS4); break;
      }
    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void FlashChipDisable(void) 
  {
  output_high(EECS1);
  output_high(EECS2);
  output_high(EECS3);
  output_high(EECS4);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
bool check_flash_erased(void)
  {
  u8 buffer[256];
  u32 index, count;
  FlashReadData(0, 256, buffer);
  for (count = 0, index = 0; index < 256; index++)
    {
    if (buffer[index] == 0xFF)
      {
      count++;
      }
    }
  if (count == 256)
    {
    PC_send_string("\r\nSim flash is erased\r\n");
    return 1;
    }
  return 0;
  }


#if __EEPROM_TYPE__== EEPROM_TYPE_SPI 

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void enable_ext_eeprom_write(void)
  {
  output_high(SF_WP);
  delay_us(10);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void disable_ext_eeprom_write(void)
  {
  output_low(SF_WP);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void wait_eeprom_write(u8 select)
  {
//  return;
//  while (eeprom_write_timer) // wait for previous eeprom write cycle to end
//    {
//    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void write_ext_eeprom(u32 address, u8 *data, u16 len)
  {
  u32 segments = 1;
  u32 length = len;
  if ((address & (NOR_PAGE_SIZE-1)) + len > NOR_PAGE_SIZE) // crossing page boundary?
    {
    segments++;
    length = NOR_PAGE_SIZE - (address & (NOR_PAGE_SIZE-1));
    }
  while (segments--)
    {
    FlashWriteData(address, length, data);
    delay_ms(10);
    if (segments)
      {
      data += length;
      address += length;
      length = len - length;
      }
    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void read_ext_eeprom(u8 caller, u32 address, u8 *data, u16 len)
  {
  u8 buf[128];
  if ((address & 0xFF) == 0xFF) // last byte in page?
    {
    FlashReadData(address-1, 2, buf);
    address++;                  // skip to next page
    len--;                      // 1st byte already in buf
    data[0] = buf[1];           // transfer 1st byte
    data++;
    FlashReadData(address, len, buf);
    memcpy(data, buf, len);
    return;
    }
  FlashReadData(address, len, data);
  }


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void check_read_write(u8 *TxBuff, u8 *RxBuff, u32 TxLen, u32 RxLen)
  {
  HAL_SPI_Transmit(&hspi1, TxBuff, TxLen, TxLen);
  if (RxLen)
    {
    HAL_SPI_TransmitReceive(&hspi1, TxBuff, RxBuff, RxLen, 100);
    }
  delay_us(1);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void check_ee_type_read(void)
  {
  u32 u32Add = 131072 - 256; // end of 515K byte space - 256 bytes
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_READ_DATA_CMND;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
//  g_u8SpiOneTxBuff[FLASH_SPI_4OFFSET_DATA] = 0;
  g_u32SpiOneTxLen = FLASH_SPI_SERVICE_INFO;
  g_u32SpiOneRxLen = 8;
  
//  g_pFlashReadDestPtr = DestBuffer;

  output_low(EECS4);
  check_read_write(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  output_high(EECS4);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void check_ee_type_write(void)
  {
  u32 u32Add = 131072 - 256; // end of 515K byte space - 256 bytes
  g_u8SpiOneTxBuff[FLASH_SPI_OFFSET_CMND ] = FLASH_WRITE_PAGE_CMND;
  g_u8SpiOneTxBuff[M95M01_ADDR_TOP] = (u8)(u32Add >> 16);
  g_u8SpiOneTxBuff[M95M01_ADDR_MID] = (u8)(u32Add >> 8);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT] = (u8)(u32Add >> 0);
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT+1] = 1;
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT+2] = 2;
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT+3] = 3;
  g_u8SpiOneTxBuff[M95M01_ADDR_BOT+4] = 7;
//  g_u8SpiOneTxBuff[FLASH_SPI_4OFFSET_DATA] = 0;
  g_u32SpiOneTxLen = FLASH_SPI_SERVICE_INFO + 4;
  g_u32SpiOneRxLen = 0;
  
//  g_pFlashReadDestPtr = DestBuffer;

  output_drive(SF_WP);
  output_high(SF_WP);

  output_low(EECS4);
  check_read_write(g_u8SpiOneTxBuff, g_u8SpiOneRxBuff, g_u32SpiOneTxLen, g_u32SpiOneRxLen); 
  output_high(EECS4);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void check_ee_type(void)
  {
  ee_type = 4;
  check_ee_type_read();
  if (g_u8SpiOneRxBuff[0] == 1 && g_u8SpiOneRxBuff[3] == 7) 
    {
    ee_type = 1;
    }
  else
    {
    check_ee_type_write();
    check_ee_type_read();
    if (g_u8SpiOneRxBuff[0] == 1 && g_u8SpiOneRxBuff[3] == 7) 
      {
      ee_type = 1;
      }
    }
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void init_eeprom(void)
  {
  u8 buf[32];
  u32 addr = 0, len = 8;
  InitExtFlash();
//  delay_us(10);
//  FlashReadChipId(buf);
//  delay_us(500);
//  FlashWriteChipId(addr, len, buf);
//  check_ee_type();

  __NOP();
  }

#endif


