/*
#define MOSI        PIN_C2
#define MISO        PIN_C1
#define HOLD        PIN_E2
#define MEXP_CS1    PIN_E1
#define MEXP_CS2    PIN_E0
#define SCLK        PIN_B4
*/

#define SW_SPI

#ifndef SW_SPI
#use spi(DI=PIN_D1, DO=PIN_D1, CLK=PIN_D4, BITS=8)
#endif

#define FLASH_BLOCK_SIZE 512

#define SF_CS      MEXP_CS1 //   PIN_E1
#define SRAM_CS    MEXP_CS2 //   PIN_E0
#define SF_RST     PIN_E5


/*============================================================================
READ   00000011   Read data from memory array beginning at selected address
WRITE  00000010   Write data to memory array beginning at selected address
RDSR   00000101   Read STATUS register
WRSR   00000001   Write STATUS register
*/


void init_spi(void)
  {
#ifdef SW_SPI
  output_drive(SF_RST);
  output_drive(SF_CS);
  output_low(SF_CS);
  delay_cycles(50);
  output_low(SF_RST);
  delay_us(100);
  output_high(SF_RST);
  output_high(SF_CS);
#else
  setup_spi(SPI_MASTER|SPI_CLK_DIV_4);
#endif
  }


//==============================================================
//==============================================================
//==============================================================

//---------------------------------------------------------------------------
//  Copyright (c) DVSoft 2004
//  email dvsoft@club-internet.fr
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the
//  Free Software Foundation.
//
//  This program is distributed in the hope it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANDIBILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  Originale Unit Name : FLASH_Driver.c
//  Author              : Alain
//  Date                : 15-Avril-2004
//  Project             : YA1-LOGGER-V3
//  Version             : 0.9.0.1
//  Revision            :
//
//----------------------------------------------------------------------------
//
//  DataFlash Driver AT45DB321B for CCS compiler
//
//  Inactive Clock Polarity High
//  level convertor 74F07 Hex inverter/buffer drivers (open-collector)
//  Pull-Up resistor 2K2
//
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Pin Definitions
//
#define FLASH_CS      SF_CS

//----------------------------------------------------------------------------
// Dataflash Config
//
#define FLASH_BlockSize  512       // FLASH_ block Size
#define FLASH_PageSize   528       // FLASH_ Page Size
//const UINT FLASH_NbrPage   = 8192;      // FLASH_ Nb Page

//---------------------------------------------------------------------------
// Dataflash Macro
//
// see Dataflash AC Characteristics
// tCSS /CS Setup Time     250 ns
//
#define FLASH_Select()\
        output_low(FLASH_CS);\
        delay_cycles(8)               // 300 ns PIC18F452 at 40Mhz

#define FLASH_UnSelect()\
        output_high(FLASH_CS);\
        delay_cycles(8)               // 300 ns PIC18F452 at 40Mhz

//---------------------------------------------------------------------------
// DataFlash PageCount
//
UINT   FLASH_PageCnt;
#byte   FLASH_PageCnt_L=FLASH_PageCnt
#byte   FLASH_PageCnt_H=FLASH_PageCnt+1

//---------------------------------------------------------------------------
// DataFlash ByteCount
//
UINT   FLASH_ByteCnt;
#byte   FLASH_ByteCnt_L=FLASH_ByteCnt
#byte   FLASH_ByteCnt_H=FLASH_ByteCnt+1

//---------------------------------------------------------------------------
// DataFlash Address
//
UINT   FLASH_Address;
#byte   FLASH_Address_L=FLASH_Address
#byte   FLASH_Address_H=FLASH_Address+1

//---------------------------------------------------------------------------
// DataFlash Dump PageCnt
//
UINT   FLASH_DumpPageCnt;

//---------------------------------------------------------------------------
// DataFlash Full Flags
//
BOOLEAN FLASH_Full;

//---------------------------------------------------------------------------
// DataFlash Buffer Select for Double Buffering
//
BOOLEAN FLASH_Buffer_2_Active;

//---------------------------------------------------------------------------
// DataFlash Driver State
//
BOOLEAN FLASH_DriverRun;

//---------------------------------------------------------------------------
// Function : FLASH_ReadStatus(void)
// Input    :
//      void
// return   :
//      int     The Status register value
// Modify   :
//      void
//
// Note     :
//
//

#define SPI_BIT(b, im)      \
      if (b & im)           \
        output_high(MOSI);  \
      else                  \
        output_low(MOSI);   \
      output_high(SCLK);    \
      if (input(MISO))      \ 
        inp |= im;          \
      output_low(SCLK);

#ifdef SW_SPI
UCHAR spi_xfr(UCHAR out)
  {
  UINT inp;
  inp = 0;
  SPI_BIT(out, 0x80);
  SPI_BIT(out, 0x40);
  SPI_BIT(out, 0x20);
  SPI_BIT(out, 0x10);
  SPI_BIT(out, 0x08);
  SPI_BIT(out, 0x04);
  SPI_BIT(out, 0x02);
  SPI_BIT(out, 0x01);
  return inp;
  }
#else
UCHAR spi_xfr(UCHAR out)
  {
  return spi_xfer(out);
  }
#endif

/*
UCHAR spi_xfr(UCHAR out)
  {
  UINT count, inp;
  for (inp = 0, count = 0; count < 8; count++)
    {
    if (out & 0x80)
      {
      output_high(MOSI);
      }
    else
      {
      output_low(MOSI);
      }
    out <<= 1;
//    delay_cycles(1);
    output_high(SCLK);
    inp <<= 1;
//    delay_cycles(1);
    if (input(MISO))
      {
      inp |= 1;
      }
    delay_cycles(1);
    output_low(SCLK);
    }
  return inp;
  }
*/


#separate
UCHAR FLASH_ReadStatus(void)
  {
  UCHAR Status;

  FLASH_Select();                                                                  //--- Select
  spi_xfr(0xD7);                                                                //--- Command Read Status
  Status = spi_xfr(0xFF);                                                           //--- Status Result
  FLASH_UnSelect();                                                                //--- Unselect
  return (Status);
  }

//---------------------------------------------------------------------------
// DataFlash PIN 1 Ready/Busy
bit FLASH_IsReady(void)
  {
  UCHAR status;
  status = FLASH_ReadStatus();                                                     //--- Read  2 just in case
  return (status & 0x80) != 0;
  }

//---------------------------------------------------------------------------
// Function : FLASH_PageToRam(UINT PageNum)
// Input    :
//      UINT PageNum       Page index to load in memory buffer
// return   :
//      void
// Modify   :
//      Load the memory page 'PageNum' in current buffer (1 or 2)
//
// Note     :
//      Change the active buffer1 or Buffer2
//
#separate
void FLASH_PageToRam(UINT PageNum)
  {

  FLASH_Address = (PageNum << 2);                                                  //--- Active buffer to main memory page
  while (!FLASH_IsReady());
  FLASH_Select();                                                                  //--- Select
  if (FLASH_Buffer_2_Active)
    {                                                                           //--- Buffer 2 is Active Buffer
    spi_xfr(0x53);                                                              //--- Memory Page To Buffer 1
    FLASH_Buffer_2_Active = 0;                                                //--- Set Buffer 1 Active
    }// End IF
  else
    {                                                                           //--- Buffer 1 is Active Buffer
    spi_xfr(0x55);                                                              //--- Memory Page To Buffer 2
    FLASH_Buffer_2_Active = 1;                                                 //--- Set Buffer 2 Active
    }// End Else
  spi_xfr(FLASH_Address_H);                                                        //--- 7 high address bits
  spi_xfr(FLASH_Address_L);                                                        //--- 6 low address bits
  spi_xfr(0x00);                                                                //--- don't care 8 bits
  FLASH_UnSelect();                                                                //--- Unselect
  }

//---------------------------------------------------------------------------
// Function : FLASH_RamToPage(UINT PageNum)
// Input    :
//      UINT PageNum       Page index to store
// return   :
//      void
// Modify   :
//      Store the current memory buffer in memory page 'PageNum'
//      Change the active buffer1 or Buffer2
//
// Note     :
//
#separate
void FLASH_RamToPage(UINT PageNum)
  {

  FLASH_Address = (PageNum << 2);                                                  //--- Active buffer to main memory page
  while (!FLASH_IsReady());                                                        //--- Wait FLASH_Ready
  FLASH_Select();                                                                  //--- Select
  if (FLASH_Buffer_2_Active)
    {                                                                           //--- Use Buffer 2 ?
    spi_xfr(0x86);                                                              //--- Buffer 2 To PAGE With Built In erase
    FLASH_Buffer_2_Active = 0;                                                //--- Set Buffer 1 Active
    }// End IF
  else
    {
    spi_xfr(0x83);                                                              //--- Buffer 1 To PAGE With Built In erase
    FLASH_Buffer_2_Active = 1;                                                 //--- Set Buffer 2 Active
    }// End ELse
  spi_xfr(FLASH_Address_H);                                                        //--- 7 high address bits
  spi_xfr(FLASH_Address_L);                                                        //--- 6 low address bits
  spi_xfr(0x00);                                                                //--- don't care 8 bits
  FLASH_UnSelect();                                                                //--- Unselect
  }

//---------------------------------------------------------------------------
// Function : FLASH_InitDump(void)
// Input    :
//      void
// return   :
//      void
// Modify   :
//      Set the dataflash page dump 'FLASH_DumpPageCnt' to 0
//      Load the first memory page in memory buffer
//
// Note     :
//
#separate
void FLASH_InitDump(void)
  {

  FLASH_DumpPageCnt = 0;                                                           //--- First page to dump
  FLASH_PageToRam(FLASH_DumpPageCnt);                                                 //--- Load first page in memory buffer
  }

#separate
//---------------------------------------------------------------------------
// Function : FLASH_DumpPacket(void)
// Input    :
//      void
// return   :
//      int     0   End off memory pages
//              1   Some memory pages left
//
// Modify   :
//     increment the dataflash page dump 'FLASH_DumpPageCnt'
//
// Note     :
//     Load in memory buffer the next memory page
//
int FLASH_DumpNextPage(void)
  {

  FLASH_DumpPageCnt++;                                                             //--- Next Page
  if (FLASH_DumpPageCnt < FLASH_PageCnt)
    {                                            //--- Not in Last Page ?
    FLASH_PageToRam(FLASH_DumpPageCnt);                                               //--- Load in memory buffer
    return 1;                                                                   //--- OK
    }// End If
  return 0;                                                                     //--- Fin
  }

//---------------------------------------------------------------------------
// Function : FLASH_DumpPacket(void)
// Input    :
//      void
// return   :
//      void
//
// Modify   :
//      void
//
// Note     :
//     Dump the complet buffer to the serial UART
//     Send only the data in Dataflash buffer
//
#separate
void FLASH_DumpPacket(void)
  {
  UCHAR  TxBuffer;
  UINT ByteCnt;

  while (!FLASH_IsReady());                                                        //--- Wait FLASH_Ready
  FLASH_Select();                                                                  //--- Initialisation
  if (FLASH_Buffer_2_Active)                                                      //--- Buffer 2 is Active ?
    spi_xfr(0x56);
  else                                                                          //--- Buffer 1 is Active
    spi_xfr(0x54);
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- High Byte address
  spi_xfr(0x00);                                                                //--- Low Byte address
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  ByteCnt = FLASH_PageSize;                                                        //--- Byte Count
  do
    {                                                                           //--- Load In Buffer
    TxBuffer = spi_xfr(0xFF);                                                       //--- Read One Byte
    putc(TxBuffer);                                                             //--- Put in serial UART
    }
  while (--ByteCnt);
  FLASH_UnSelect();                                                                //--- Unselect
  }

//---------------------------------------------------------------------------
// Function : FLASH_Open(void)
// Input    :
//      void
// return   :
//      void
//
// Modify   :
//      Set the flag 'FLASH_DriverRun' to TRUE  if ok
//                                   FALSE if not Ok
//      Set the Flag 'FLASH_Full' to TRUE if (FLASH_PageCnt == FLASH_NbrPage)
//
//      Set the Byte Count 'FLASH_ByteCnt' to 0
//
//      Set the Dump page counter 'FLASH_DumpPageCnt' to 0
//
// Note     :
//
#separate
void FLASH_Open(void)
  {
  UCHAR Status;

//  setup_spi(SPI_MASTER|SPI_H_TO_L|SPI_CLK_DIV_4|SPI_SAMPLE_AT_END);             //--- Setup du SPI
  FLASH_Buffer_2_Active = 0;                                                  //--- Buffer 1 Is Active Buffer
  output_high(SF_RST);
  FLASH_ReadStatus();                                                              //--- Read the status register
  Status = FLASH_ReadStatus();                                                     //--- Read  2 just in case

                //    AT45DB321B  (TABLE 4 and 5 page 8) Using Atmel's DataFlash document
                //
                //     bit 7      bit6   bit5 bit4 bit3 bit2  bit1 bit0
                //    Rdy/Busy  Compare     Density code      reserved
                //       1         0      1    1    0    1     0    0
                //

  if ((Status & 0x80) && ((Status & 0x3C) == 0b00110100))
    {                                                                           //--- Status OK ?
    FLASH_DriverRun = TRUE;                                                        //--- DataFlash OK
    }// End If
  else
    FLASH_DriverRun = FALSE;                                                       //--- DataFlash HS ?
  }

//---------------------------------------------------------------------------
// Function : FLASH_WriteToFlash(UCHAR Data)
// Input    :
//      UCHAR Data      Data to flash
// return   :
//      void
//
// Modify   :
//      Set the flag FLASH_Full if last page
//
// Note     :
//      Automatic store to memory area with built-in-erase when the ram buffer
// is full. Use double bufferring for fast operation
//
#separate
void FLASH_WriteToFlash(UCHAR Data)
  {

  if (!FLASH_DriverRun)                                                            //--- Drivers RUN ?
      return;
  if (FLASH_Full)                                                                  //--- FLASH_ Full
      return;
  while (!FLASH_IsReady());                                                        //--- Wait Ready
  FLASH_Select();                                                                  //--- Start Write Ram
  if (FLASH_Buffer_2_Active)                                                      //--- Write To Buffer 1 ?
    spi_xfr(0x87);
  else                                                                          //--- Write to Buffer 2
    spi_xfr(0x84);
  spi_xfr(0x00);                                                                //--- 8 Don't care Bit
  spi_xfr(FLASH_ByteCnt_H);                                                        //--- High Byte address
  spi_xfr(FLASH_ByteCnt_L);                                                        //--- Low Byte address
  spi_xfr(Data);                                                                //--- Write The Value
  FLASH_UnSelect();                                                                //--- UnSelect
  }

//---------------------------------------------------------------------------
// Function : FLASH_WriteDataToBuffer(ULONG Addr, UCHAR *Data, UINT Length)
// Input    :
//      UCHAR Data      Data to flash
// return   :
//      void
//
// Note     :
//      Automatic store to memory area with built-in-erase when the ram buffer
// is full. Use double bufferring for fast operation
//
#separate
void FLASH_WriteDataToBuffer(UINT Addr, UCHAR *Data, UINT Length)
  {
  UCHAR x;
  if (!FLASH_DriverRun)                                                            //--- Drivers RUN ?
      return;
  while (!FLASH_IsReady());                                                        //--- Wait Ready
  FLASH_Select();                                                                  //--- Start Write Ram
  if (FLASH_Buffer_2_Active)                                                      //--- Write To Buffer 1 ?
    spi_xfr(0x87);
  else                                                                          //--- Write to Buffer 2
    spi_xfr(0x84);
  spi_xfr(0x00);                                                                //--- 8 Don't care Bit
  spi_xfr(Addr >> 8);                                                           //--- High Byte address
  spi_xfr(Addr & 255);                                                          //--- Low Byte address
  do
    {
    spi_xfr(*Data++);                                                                    // send data to FLASH RAM page
    }
  while (--Length);
  FLASH_UnSelect();                                                                //--- UnSelect
  }

//---------------------------------------------------------------------------
// Function : FLASH_WriteDataToBuffer(UINT Addr, UCHAR *Data, UINT Length)
// Input    :
//      void
// return   :
//      void
//
// Modify   :
//      void
//
// Note     :
//     Dump the complet buffer to the serial UART
//     Send only the data in Dataflash buffer
//
#separate
void FLASH_ReadDataFromBuffer(UINT Addr, UCHAR *Data, UINT Length)
  {
  while (!FLASH_IsReady());                                                        //--- Wait FLASH_Ready
  FLASH_Select();                                                                  //--- Initialisation
  if (FLASH_Buffer_2_Active)                                                      //--- Buffer 2 is Active ?
    spi_xfr(0xD6);
  else                                                                          //--- Buffer 1 is Active
    spi_xfr(0xD4);
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(Addr >> 8);                                                           //--- High Byte address
  spi_xfr(Addr & 255);                                                          //--- Low Byte address
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  while (Length--)
    {
    *Data++ = spi_xfr(0xFF);
    }

  FLASH_UnSelect();                                                                //--- Unselect
  }

//---------------------------------------------------------------------------
// Function : FLASH_ModifyDataInPage(UINT PageNum, UINT Addr, UCHAR *Data, UINT Length)
void FLASH_ModifyDataInPage(UINT PageNum, UINT Addr, UCHAR *Data, UINT Length)
  {
  if (Length < FLASH_BLOCK_SIZE)
    FLASH_PageToRam(PageNum);
  FLASH_WriteDataToBuffer(Addr, Data, Length);
  FLASH_RamToPage(PageNum);
  }

//---------------------------------------------------------------------------
// Function : FLASH_GetDataFromPage(UINT PageNum, UINT Addr, UCHAR *Data, UINT Length)
void FLASH_GetDataFromPage(UINT PageNum, UINT Addr, UCHAR *Data, UINT Length)
  {
  FLASH_PageToRam(PageNum);
  FLASH_ReadDataFromBuffer(Addr, Data, Length);
  }

//---------------------------------------------------------------------------
// Function : FLASH_WriteDataToBuffer(ULONG Addr, UCHAR *Data, UINT Length)
#define PUT_ZERO x = spi_xfr(0);
#define PUT_VALUE(v) x = spi_xfr(v);

#separate
void FLASH_ClearDataInBuffer(UINT Addr, UINT Length)
  {
  UCHAR x;
  if (!FLASH_DriverRun)                                                            //--- Drivers RUN ?
      return;
  while (!FLASH_IsReady());                                                        //--- Wait Ready
  FLASH_Select();                                                                  //--- Start Write Ram
  if (FLASH_Buffer_2_Active)                                                      //--- Write To Buffer 1 ?
    spi_xfr(0x87);
  else                                                                          //--- Write to Buffer 2
    spi_xfr(0x84);
  spi_xfr(0x00);                                                                //--- 8 Don't care Bit
  spi_xfr(Addr >> 8);                                                           //--- High Byte address
  spi_xfr(Addr & 255);                                                          //--- Low Byte address
  do
    {
    PUT_ZERO;                                                                    // send data to FLASH RAM page
    }
  while (--Length);
  FLASH_UnSelect();                                                                //--- UnSelect
  }

//---------------------------------------------------------------------------
// Function : FLASH_ClearDataInPage(UINT PageNum, UINT Addr, UCHAR *Data, UINT Length)
void FLASH_ClearDataInPage(UINT PageNum, UINT Addr, UINT Length)
  {
  FLASH_PageToRam(PageNum);
  FLASH_ClearDataInBuffer(Addr, Length);
  FLASH_RamToPage(PageNum);
  }


//---------------------------------------------------------------------------
#separate
void FLASH_DirectReadDataFromFLASH(ULONG Addr, UCHAR *Data, UINT Length)
  {
  UCHAR len, block;
  len = Length;
  while (!FLASH_IsReady());                                                        //--- Wait FLASH_Ready
  FLASH_Select();                                                                  //--- Initialisation
  spi_xfr(0xD2);
  spi_xfr(make8(Addr, 2));                                                                //--- 8 Dont'care Byte
  spi_xfr(make8(Addr,1));                                                           //--- High Byte address
  spi_xfr(Addr & 255);                                                          //--- Low Byte address
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  while (len--)
    {
    *Data++ = spi_xfr(0xFF);
    }
  FLASH_UnSelect();                                                                //--- Unselect
  }


//---------------------------------------------------------------------------
void FLASH_ResetPages(UINT PageNum, UINT pages)
  {
  UCHAR x, Length;
  if (!FLASH_DriverRun)                                                            //--- Drivers RUN ?
      return;
  while (!FLASH_IsReady());                                                        //--- Wait Ready
  FLASH_Select();                                                                  //--- Start Write Ram
  FLASH_Buffer_2_Active = 0;
  spi_xfr(0x84);
  spi_xfr(0x00);                                                                //--- 8 Don't care Bit
  spi_xfr(0);                                                           //--- High Byte address
  spi_xfr(0);                                                          //--- Low Byte address
  Length = 64;
  do
    {
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    PUT_VALUE(0xFF);                                                                    // send data to FLASH RAM page
    }
  while (--Length);
  FLASH_UnSelect();                                                                //--- UnSelect
  while (pages--)
    {
    FLASH_Buffer_2_Active = 0; // remain on same RAM page
    FLASH_RamToPage(PageNum++);
    }
  }

//---------------------------------------------------------------------------
#separate
void FLASH_ReadSecurityPage(UCHAR *Data)
  {
  UCHAR len = 128;
  while (!FLASH_IsReady());                                                        //--- Wait FLASH_Ready
  FLASH_Select();                                                                  //--- Initialisation
  spi_xfr(0x77);
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  spi_xfr(0x00);                                                                //--- 8 Dont'care Byte
  while (len--)
    {
    *Data++ = spi_xfr(0xFF);
    }
  FLASH_UnSelect();                                                                //--- Unselect
  }

//-----------------------------------------------------------------------------

UINT CalculateReverseCRC(UINT *Buffer, UINT Len)
  {
  UINT CRCres, V1;
  
  for (CRCres = 0; Len; )
    {
    V1 = make8(CRCres, 1) ^ Buffer[--Len];
    CRCres = (UINT)((CRCres << 8) ^ CCITT_CRC_TAB[V1]);
    }
  return (CRCres);
  }

//---------------------------------------------------------------------------
#separate
UINT FLASH_check_device_validity(void)
  {
  UCHAR buf[128];
  UINT crc, rcrc, sec_crc, sec_rcrc;;
  FLASH_ReadSecurityPage(buf);
  crc = CalculateCRC(buf+64, 20);
  rcrc = CalculateReverseCRC(buf+89, 22);
  sec_crc = make16(buf[5],buf[1]);
  sec_rcrc = make16(buf[2],buf[11]);
  if (crc == sec_crc && rcrc == sec_rcrc)
    {
    return 1;
    }
  return 0;
  }  
  
//---------------------------------------------------------------------------
#separate
void FLASH_fill_security_page(void)
  {
  UCHAR buf[128];
  UINT crc, rcrc, sec_crc, sec_rcrc;;
  FLASH_ReadSecurityPage(buf);
  crc = CalculateCRC(buf+64, 20);
  rcrc = CalculateReverseCRC(buf+89, 22);
  buf[5] = crc >> 8;
  buf[1] = crc & 255;
  buf[2]  = rcrc >> 8;
  buf[11] = rcrc & 255;
  }    
  
//---------------------------------------------------------------------------
#separate
UINT FLASH_check_device_presence(void)
  {
  UCHAR buf[128];
  UINT FFs = 0, zeros = 0, count;
  FLASH_ReadSecurityPage(buf);
  for (count = 0; count < 128; count++)
    if (buf[count] == 0xFF)
      FFs++;
    else if (buf[count] == 0)
      zeros++;
  if (FFs == 128 || zeros == 128)
    {
    return 0;
    }
  return 1;
  }  
  
  