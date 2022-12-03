#include "main.h"

#include "setup.h"

#define __JERUSALEM_MODE__ 0


u8 zeros[MAX_DOORS], ones[MAX_DOORS];
u32 raw_wiegand;

u32 WiegandR2D1b_level[MAX_DOORS] = { 1,1,1,1 };
u32 WiegandReader2[MAX_DOORS];  // D1 detected on reader #2
u32 Wiegand2Readers[MAX_DOORS]; // door is defined having 2 readers
u32 CheckWiegandRdrD1bCtr;

extern u32 dip_switches;
extern TIM_HandleTypeDef htim1;

//--------------------------------------------------------------
// wiegand reader functions
//--------------------------------------------------------------


//=============================================================================================
void wiegand_d1(u32 reader)
  {
  if ((reader_bitmask[reader] >>= 1) == 0)
    {
    reader_bitmask[reader] = 0x80;
    WGND_kepress[reader] = *reader_byteptr[reader];
    reader_byteptr[reader]++;
    }
  Timer_1mS_Count[reader] = 0; // start timeout counter from this bit
  BitCnt[reader]++;
  }

//=============================================================================================
// called every 80uS by TIM1
u32 maxWtime;
void CheckWiegandRdrD1b(void)
  {
//  CheckWiegandRdrD1bCtr++;
//  output_toggle(SCL);
//  u32 t0, t1;
//  t0 = TIM1->CNT;
  if (Wiegand2Readers[0])
    {
    if (input(RDR1_D1b) != 0)
      {
      if (WiegandR2D1b_level[0])
        {
        WiegandR2D1b_level[0] = 0;
        WiegandReader2[0] = 1;
        wiegand_d1(0);
        }
      }
    else
      {
      WiegandR2D1b_level[0] = 1;
      }
    }
  if (Wiegand2Readers[1])
    {
    if (input(RDR2_D1b) != 0)
      {
      if (WiegandR2D1b_level[1])
        {
        WiegandR2D1b_level[1] = 0;
        WiegandReader2[1] = 1;
        wiegand_d1(1);
        }
      }
    else
      {
      WiegandR2D1b_level[1] = 1;
      }
    }
  if (Wiegand2Readers[2])
    {
    if (input(RDR3_D1b) != 0)
      {
      if (WiegandR2D1b_level[2])
        {
        WiegandR2D1b_level[2] = 0;
        WiegandReader2[2] = 1;
        wiegand_d1(2);
        }
      }
    else
      {
      WiegandR2D1b_level[2] = 1;
      }
    }
  if (Wiegand2Readers[3])
    {
    if (input(RDR4_D1b) != 0)
      {
      if (WiegandR2D1b_level[3])
        {
        WiegandR2D1b_level[3] = 0;
        WiegandReader2[3] = 1;
        wiegand_d1(3);
        }
      }
    else
      {
      WiegandR2D1b_level[3] = 1;
      }
    }
//  t1 = TIM1->CNT - t0;
//  if (t1 > maxWtime)
//    {
//    maxWtime = t1;
//    }
  }

//=============================================================================================
void wiegand_d0(u32 reader)
  {
  // data input is zero. simply move to next bit
  *reader_byteptr[reader] |= reader_bitmask[reader];

  if ((reader_bitmask[reader] >>= 1) == 0)
    {
    reader_bitmask[reader] = 0x80;
    WGND_kepress[reader] = *reader_byteptr[reader];
    reader_byteptr[reader]++;
    }
  Timer_1mS_Count[reader] = 0; // start timeout counter from this bit
  BitCnt[reader]++;
  }

//=============================================================================================
void WGND_init(u8 reader)
  {
  reader_bitmask[reader] = 0x80;
  BitCnt[reader]= 0;
  WGND_kepress[reader] = 0xFF;
  reader_data[reader][8] = 0;
  reader_data[reader][7] = 0;
  reader_data[reader][6] = 0;
  reader_data[reader][5] = 0;
  reader_data[reader][4] = 0;
  reader_data[reader][3] = 0;
  reader_data[reader][2] = 0;
  reader_data[reader][1] = 0;
  reader_data[reader][0] = 0;
  reader_byteptr[reader] = &reader_data[reader][0];
  zeros[reader] = 0;
  ones[reader] = 0;
//  WiegandReader2[reader] = 0;
  if (doors[reader].Flags & LFLAG_2RDR_SINGLE_DOOR)
    {
    if (Wiegand2Readers[reader] == 0)
      {
      __HAL_TIM_ENABLE(&htim1);
      __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
      }
    Wiegand2Readers[reader] = 1;
    }
  else
    {
    Wiegand2Readers[reader] = 0;
    }
  }


//=============================================================================================
u32 extract_wiegand_key(u32 first, u32 digits, u8 *data)
  {
//  u16 idx;
  u32 number = 0L, reader_mode;
  u8 digit, mask;
  first -= 1;
  number = 0;
  raw_wiegand = make32(data[3], data[2], data[1], data[0]);

#if __JERUSALEM_MODE__ == 1
    number = make32(0,0, data[2], data[3]); // HID Jerusalem municipality?
    return number;
#endif
    
  while (first >= 8) // skip to first byte of data
    {
    first -= 8;
    data++;
    }
  mask = 1 << (7 - (first & 7));
  digit = *data++;
//  idx = 0;
  while (digits)
    {
      {
      number <<= 1;
      if (digit & mask)
        number |= 1;
      mask >>= 1;
      if (!mask)
        {
        mask = 0x80;
        digit = *data++;
        }
      digits--;
      }
    }
  return number;
  }


//=============================================================================================
void process_pin(u32 reader, u32 tag)
  {
  new_key[reader] = tag;
  led_period[reader] = 20;
  led_onoff(reader, 1);
  UserKeyReady[reader] = KEY_SEND_TIMES;
  KeySource[reader] = 0;
  init_pin_only_wait(reader);
  WGND_init(reader);
  }

//=============================================================================================
u8 WGND_wait(u32 reader)
  {
  u32 digit;//, bytes;
  u16 pincode;
  u8 kswitch[4];
  u32 tag;
  if (!BitCnt[reader])
    {
    Timer_1mS_Count[reader] = 0;
    }
  else if (Timer_1mS_Count[reader] > 20)
    {
    if (BitCnt[reader] < 26)
      {
      if ((pin_only_mode(reader) || wait_wiegand_digit[reader]) && BitCnt[reader] == 4)
        {
        digit = reader_data[reader][0];
        reader_data[reader][0] = 0;
        digit >>= 4;
        BitCnt[reader] = 0;
        if (digit < 10) 
          {     // 0 to 9
#if __LOCKER_SYSTEM__ == 1      
          add_digit_to_lock_number(digit); // collect digits for locker number ID
#endif
          digit <<= (8 - ++collected_digits[reader]) * 4;
          collected_pin[reader] |= digit;
          }
        else
          {     // this is either * or #
#if __LOCKER_SYSTEM__ == 1      
          process_locker_number();
#else          
          while (collected_digits[reader] < 9)
            {
            digit = (u32)15 << (8 - ++collected_digits[reader]) * 4;
            collected_pin[reader] |= digit;
            }
          digit = collected_pin[reader];
          if (pin_only_mode(reader))
            {
            process_pin(reader, digit);
            return 1;
            }
          pincode = CalculateCRC((u8 *)&digit, 4);
          if (pincode == pin_key_rec[reader].PIN)
            {
            process_key(reader, pin_key_rec[reader].Code, 1);
            }
          else
            {
            generate_event(reader, pin_key_rec[reader].ID, pin_key_rec[reader].Code, EVT_wrong_PIN_code);
            }
#endif          
          clear_pin_wait(reader);
          }
        }
      
      WGND_init(reader);
      return 0;
      }
    else if (wait_wiegand_digit[reader])
      {
      clear_pin_wait(reader);
      }
    
    for (u32 idx = 0; idx < 8; idx++)
      {
      reader_data[reader][idx] ^= 0xFF;
      }
    
    tag = extract_wiegand_key(doors[reader].First_digit, doors[reader].Number_of_digits, &reader_data[reader][0]);
    if (mul_t_lock)
      {
      kswitch[0] = make8(tag, 2);
      kswitch[1] = make8(tag, 1);
      kswitch[2] = make8(tag, 0);
      kswitch[3] = make8(tag, 3);
      tag = make32(kswitch[3],kswitch[2],kswitch[1],kswitch[0]);
      }
    new_key[reader] = tag;
    led_period[reader] = 20;
    led_onoff(reader, 1);
    
     UserKeyReady[reader] = KEY_SEND_TIMES;
    KeySource[reader] = 0;
    WGND_init(reader);
    return 1;
    }
  return 0;
  }

//=============================================================================================
bool Reader2IsSource(u32 door)
  {
  if (Wiegand2Readers[door] && WiegandReader2[door])
    {
    WiegandReader2[door] = 0;
    return 1;
    }
  return 0;
  }

//=============================================================================================
void ClearReader2(u32 door)
  {
  WiegandReader2[door] = 0;
  }