#include "main.h"

#include "setup.h"
#include "string.h"

void MCR1_Enable();

//--------------------------------------------------------------------------
// initialize card reading
void MCR1_ResetSearch(void)
  {
  MCR_SrchForB0 = 1;     // first search for STX
  MCR_CollectData0 = 0;  // reset data collection flag
  MCR_InitStamp0 = 1;    // cause handler to time stamp this reading
  MCR_NibbleCnt0 = 0;        // zero out data collection variables
  MCR_Time0 = 0;
  BitCnt[0] = 0;
  MCR_Nibble0 = 0;
  MCR_OddParity0 = 0;
  MCR_T1chr[0] = 0;
  raw_start[0] = 1;
  TrackOneDetected[0] = 0;
  MCR_Ones0 = 0;
  }

//--------------------------------------------------------------------------
// interrupt service routine
void MCR1_MagData(void)
  {
  MCR_InData0 = input(MCR_DATA0);       // get card data
  TMR_10mS_Count[0] = 0;
  MCR_Time0 = 0;

  if (MCR_SrchForB0)               // are we still looking for STX?
    {
    MCR_Nibble0 >>= 1;                 // YES. make room for next bit in track II data
    MCR_T1chr[0] >>= 1;                    // dito for track I data
    if (!MCR_InData0)                  // invert input data
      {
      MCR_Nibble0 |= 0x10;             // track II data (4 data bits[0] = parity)
      MCR_T1chr[0] |= 0x40;                // track I data  (6 data bits + parity)
      }
    if ((MCR_Nibble0 & 0x1F) == 0x0B)  // is STX detected?
      {                           // YES.
      MCR_SrchForB0 = 0;           // switch to collecting data
      MCR_CollectData0 = 1;
      BitCnt[0] = 0;
      MCR_OddParity0 = 0;
      TrackOneDetected[0] = 0;
      }
    else if ((MCR_T1chr[0] & 0x7F) == 0x45) // is start sentinel detected?
      {
      MCR_SrchForB0 = 0;           // switch to collecting data
      MCR_TrackOne0 = 1;
      BitCnt[0] = 0;
      MCR_OddParity0 = 0;
      TrackOneDetected[0] = 1;
      }
    }
  else if (MCR_CollectData0)          // are we collecting data?
    {                           // YES.
    MCR_Nibble0 >>= 1;               // shift nibble and
    if (!MCR_InData0)                // insert a "1" if input is "0"
      {
      MCR_OddParity0++;              // increment "1 counter
      MCR_Nibble0 |= 0x10;           //
      }
    if (++BitCnt[0] == 5)          // 5 bits collected?
      {                         // YES.
      if (!Bit_Test(MCR_OddParity0,0))
        {
data_error:
        MCR1_ResetSearch();      // if parity is wrong, reset search
        return;
        }

      reader_data[reader][MCR_NibbleCnt0] = MCR_Nibble0 & 0x0F;
      BitCnt[0] = 0;               // clear nibble variables
      MCR_OddParity0 = 0;
      if (++MCR_NibbleCnt0 > MAX_READER_DATA_LEN - 2)
        goto data_error;
      if ((MCR_Nibble0 & 0x1F) == 0x1F)
        {                       // if ETX detected then no more data
endofdata:
        if (MCR_NibbleCnt0 < 5)
          goto data_error;
        MCR_CollectData0 = 0;
        disable_interrupts(INT_EXT1);
        CardReady[0] = 1;       // key is ready for processing
        }
      }
    }
  else if (MCR_TrackOne0) // track one mode
    {
    if (MCR_InData0)                // add new bit to CRC register
      MCR_OddParity0++;
    MCR_T1chr[0] >>= 1;                 // make room for next bit in shift register
    if (!MCR_InData0)                // insert new bit
      MCR_T1chr[0] |= 0x40;
    if (++BitCnt[0] == 7)          // 7 bits collected?
      {
      if (!MCR_OddParity0)                         // YES.
        goto data_error;
      BitCnt[0] = 0;
      reader_data[reader][MCR_NibbleCnt0] = MCR_T1chr[0] & 0x3F;
      if (++MCR_NibbleCnt0 > MAX_READER_DATA_LEN - 2)
        goto data_error;
      if (MCR_T1chr[0] == 0x1F)  // end of data?
        goto endofdata;          // YES.
      }
    }

  }

//--------------------------------------------------------------------------
void MCR1_Enable(void)
  {
  CardReady[0] = 0;
  MCR1_ResetSearch();
  EXT_INT_EDGE(0, H_TO_L); // collect data following rising edge
  clear_interrupt(int_EXT1);
  enable_interrupts(INT_EXT1); // clock input
  }

//--------------------------------------------------------------------------
void MCR1_Disable(void)
  {
  disable_interrupts(INT_EXT1);
  }

//--------------------------------------------------------------------------
void MCR1_init_magcard(void)
  {
  MCR1_Enable();
  }


//--------------------------------------------------------------------------
u8 MCR1_wait_magcard(u32 reader)
  {
  u8 idx, buf[16], count, digits, digit;
  u32 number;
  if (CardReady[reader])
    {
    CardReady[reader] = 0;
    enable_interrupts(INT_EXT1); // clock input
    if (doors[0].alternate_mode) // collect raw data?
      {
      if (reader_data[reader][7] != reader_data[reader][8] || 
          reader_data[reader][7] != reader_data[reader][9] || 
          reader_data[reader][8] != reader_data[reader][9])
        {
        goto restart;
        }
      reader_data[reader][reader] = 0;
      }
    if (TrackOneDetected[reader] || doors[0].alternate_mode)
      {
      memcpy(buf, &reader_data[reader][0], MCR_NibbleCnt0);
      }
    else
      {
      idx = doors[0].first_digit - 1;
      number = 0;
      for (count = 0, digits = 28; count < doors[0].number_of_digits; count++, digits -= 4, idx++)
        {
        digit = reader_data[reader][idx];
        number |= (u32)digit << digits;
        }
      MCR_NibbleCnt1 = (MCR_NibbleCnt1 + 1) >> 1;
      }

    if (doors[0].number_of_digits < 8)
      number >>= (8 - doors[0].number_of_digits) * 4;

    memcpy(received_key0, &number, 4);
    memcpy(&new_key[reader], &number, 4);
    MCR1_Enable();      // reset card search
    UserKeyReady[reader] = KEY_SEND_TIMES;
    KeySource[reader] = 0;
    return 1;
    }
  else if (MCR_NibbleCnt0)
    if (TMR_10MS_READER0)
      {
      TMR_10MS_READER0 = 0;
      if (++MCR_Time0 > 7)
        if (doors[0].alternate_mode) // collect raw data?
          CardReady[reader] = 1;
        else
        MCR1_Enable();
      }
  return 0;
restart:
  MCR1_enable();
  return 0;
  }
