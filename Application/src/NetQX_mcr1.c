#include "main.h"

#include "setup.h"
#include "string.h"


u32 MCR_NibbleCnt1[MAX_DOORS]; //     rdr_interval            // nibble counter
u8 MCR_Nibble1[MAX_DOORS]; //        reader_byteidx               // 5 bit nibble accumulator
u8 MCR_OddParity1[MAX_DOORS]; //     ReaderFreq            // odd parity 1 counter
u32 MCR_Time1[MAX_DOORS]; //          reader_bitmask             // timeout counter
u32 MCR_Ones1[MAX_DOORS]; //          ColumnParity   /* consecutive 1 counter */

bool MCR_SrchForB1[MAX_DOORS]; //     ; // = RDRstate1.1 // wait for STX nibble
bool MCR_CollectData1[MAX_DOORS]; //  ; // = RDRstate1.2 // collect trck II data bits
bool MCR_InitStamp1[MAX_DOORS]; //    ; // = RDRstate1.3 // initialization flag
bool MCR_InData1[MAX_DOORS]; //       ; // = RDRstate1.4 // input data polarity following interrupt
bool MCR_TrackOne1[MAX_DOORS]; //     ; // = RDRstate1.5 // collect track I data bits



void MCR2_Enable();

//--------------------------------------------------------------------------
// initialize card reading
void MCR_ResetSearch(u32 reader)
  {
  MCR_SrchForB1[reader] = 1;     // first search for STX
  MCR_CollectData1[reader] = 0;  // reset data collection flag
  MCR_InitStamp1[reader] = 1;    // cause handler to time stamp this reading
  MCR_NibbleCnt1[reader] = 0;        // zero out data collection variables
  MCR_Time1[reader] = 0;
  BitCnt[reader] = 0;
  MCR_Nibble1[reader] = 0;
  MCR_OddParity1[reader] = 0;
  MCR_T1chr[reader] = 0;
  raw_start[reader] = 1;
  TrackOneDetected[reader] = 0;
  MCR_Ones1[reader] = 0;
  }

//--------------------------------------------------------------------------
// interrupt service routine
void MCR_MagData(u32 reader)
  {
  switch (reader)
    {
    case 0: MCR_InData1[reader] = input(MCR_DATA1); break;      // get card data
    case 1: MCR_InData1[reader] = input(MCR_DATA2); break;      // get card data
    case 2: MCR_InData1[reader] = input(MCR_DATA3); break;      // get card data
    case 3: MCR_InData1[reader] = input(MCR_DATA4); break;      // get card data
    }
  MCR_InData1[reader] = input(MCR_DATA1);       // get card data
  Timer_10ms_count[1] = 0;
  MCR_Time1[reader] = 0;

  if (MCR_SrchForB1[reader])               // are we still looking for STX?
    {
    MCR_Nibble1[reader] >>= 1;                 // YES. make room for next bit in track II data
    MCR_T1chr[reader] >>= 1;                    // dito for track I data
    if (!MCR_InData1[reader])                  // invert input data
      {
      MCR_Nibble1[reader] |= 0x10;             // track II data (4 data bits[1] = parity)
      MCR_T1chr[reader] |= 0x40;                // track I data  (6 data bits + parity)
      }
    if ((MCR_Nibble1[reader] & 0x1F) == 0x0B)  // is STX detected?
      {                           // YES.
      MCR_SrchForB1[reader] = 0;           // switch to collecting data
      MCR_CollectData1[reader] = 1;
      BitCnt[reader] = 0;
      MCR_OddParity1[reader] = 0;
      TrackOneDetected[reader] = 0;
      }
    else if ((MCR_T1chr[reader] & 0x7F) == 0x45) // is start sentinel detected?
      {
      MCR_SrchForB1[reader] = 0;           // switch to collecting data
      MCR_TrackOne1[reader] = 1;
      BitCnt[reader] = 0;
      MCR_OddParity1[reader] = 0;
      TrackOneDetected[reader] = 1;
      }
    }
  else if (MCR_CollectData1[reader])          // are we collecting data?
    {                           // YES.
    MCR_Nibble1[reader] >>= 1;               // shift nibble and
    if (!MCR_InData1[reader])                // insert a "1" if input is "0"
      {
      MCR_OddParity1[reader]++;              // increment "1 counter
      MCR_Nibble1[reader] |= 0x10;           //
      }
    if (++BitCnt[reader] == 5)          // 5 bits collected?
      {                         // YES.
      if ((MCR_OddParity1[reader] & 1) == 0)
        {
data_error:
        MCR_ResetSearch(reader);      // if parity is wrong, reset search
        return;
        }

      reader_data[reader][MCR_NibbleCnt1[reader]] = MCR_Nibble1[reader] & 0x0F;
      BitCnt[reader] = 0;               // clear nibble variables
      MCR_OddParity1[reader] = 0;
      if (++MCR_NibbleCnt1[reader] > MAX_READER_DATA_LEN - 2)
        {
        goto data_error;
        }
      if ((MCR_Nibble1[reader] & 0x1F) == 0x1F)
        {                       // if ETX detected then no more data
endofdata:
        if (MCR_NibbleCnt1[reader] < 5)
          {
          goto data_error;
          }
        MCR_CollectData1[reader] = 0;
//        disable_interrupts(INT_EXT3);
        CardReady[reader] = 1;       // key is ready for processing
        }
      }
    }
  else if (MCR_TrackOne1[reader]) // track one mode
    {
    if (MCR_InData1[reader])                // add new bit to CRC register
      {
      MCR_OddParity1[reader]++;
      }
    MCR_T1chr[reader] >>= 1;                 // make room for next bit in shift register
    if (!MCR_InData1[reader])                // insert new bit
      {
      MCR_T1chr[reader] |= 0x40;
      }
    if (++BitCnt[reader] == 7)          // 7 bits collected?
      {
      if (!MCR_OddParity1[reader])                         // YES.
        {
        goto data_error;
        }
      BitCnt[reader] = 0;
      reader_data[reader][MCR_NibbleCnt1[reader]] = MCR_T1chr[reader] & 0x3F;
      if (++MCR_NibbleCnt1[reader] > MAX_READER_DATA_LEN - 2)
        {
        goto data_error;
        }
      if (MCR_T1chr[reader] == 0x1F)  // end of data?
        {
        goto endofdata;          // YES.
        }
      }
    }

  }

//--------------------------------------------------------------------------
void MCR_Enable(u32 reader)
  {
  CardReady[reader] = 0;
  MCR_ResetSearch(reader);
//  EXT_INT_EDGE(3, H_TO_L); // collect data following rising edge
//  clear_interrupt(int_EXT3);
//  enable_interrupts(INT_EXT3); // clock input
  }

////--------------------------------------------------------------------------
//void MCR2_Disable(void)
//  {
//  disable_interrupts(INT_EXT3);
//  }
//
//--------------------------------------------------------------------------
void MCR_init_magcard(u32 reader)
  {
  MCR_Enable(reader);
  }


//--------------------------------------------------------------------------
u8 MCR_wait_magcard(u32 reader)
  {
  u8 idx, buf[16];
  u32 number = 0, count, digits, digit;
  if (CardReady[reader])
    {
    CardReady[reader] = 0;
//    enable_interrupts(INT_EXT3); // clock input
    if (doors[reader].Alternate_mode) // collect raw data?
      {
      if (reader_data[reader][7] != reader_data[reader][8] || 
          reader_data[reader][7] != reader_data[reader][9] || 
          reader_data[reader][8] != reader_data[reader][9])
        {
        goto restart;
        }
      reader_data[reader][0] = 0;
      }
    if (TrackOneDetected[reader] || doors[reader].Alternate_mode)
      {
      memcpy(buf, &reader_data[reader][0], MCR_NibbleCnt1[reader]);
      }
    else
      {
      idx = doors[reader].First_digit - 1;
      number = 0;
      for (count = 0, digits = 28; count < doors[reader].Number_of_digits; count++, digits -= 4, idx++)
        {
        digit = reader_data[reader][idx];
        number |= (u32)digit << digits;
        }
      MCR_NibbleCnt1[reader] = (MCR_NibbleCnt1[reader] + 1) >> 1;
      }

    if (doors[reader].Number_of_digits < 8)
      {
      number >>= (8 - doors[reader].Number_of_digits) * 4;
      }
    
    memcpy(received_key[reader], &number, 4);
    memcpy(&new_key[reader], &number, 4);
    MCR_Enable(reader);      // reset card search
    UserKeyReady[reader] = KEY_SEND_TIMES;
    KeySource[reader] = 0;
    return 1;
    }
  else if (MCR_NibbleCnt1[reader])
    {
    if ((Timer_10mS_Flags & Tmr_10mS_READER1))
      {
      Timer_10mS_Flags  &= ~(Tmr_10mS_READER1);
      if (++MCR_Time1[reader] > 7)
        {
        if (doors[reader].Alternate_mode) // collect raw data?
          {
          CardReady[reader] = 1;
          }
        else
          {
          MCR_Enable(reader);
          }
        }
      }
    }
  return 0;
restart:
  MCR_Enable(reader);
  return 0;
  }
