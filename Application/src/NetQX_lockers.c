#include "main.h"

#include "setup.h"
#include "serial.h"
#include "stdio.h"
#include "ctype.h"

#define X16PREAMB '#'

#define __STID_GOV_RDR__ 1



u32 locker_unlocked[64];

bool Subnet_char_ready(void);
u8 GetCharFromSubnet(void);
u32 extract_wiegand_key(u32 first, u32 digits, u8 *data);

u8 COM2_buf[256];

u32 previous_tag[64];
u32 tag_timeout[64];
u8  locker_status[64];
u32 tag_timeout[64];
u32 IOX16_state;
u8  prev_char2;
u32 packet_idx2;
u32 scan_reader_timeout;
u32 locker_tags[64];
u32 locker_command_sent[64];



//-----------------------------------------------------------------------------
void send_to_lockers(u32 door, u8 command, char *param)
  {
  u8 buf[22], *bp, chr;
  output_high(RS485_EN); // enable transmit
  delay_us(100);
  bp = buf;
  sprintf((char *)buf, "$$%03lu %c%s\r\n", door, command, param);
  while (*bp)
    {
    chr = *bp++;
    RS485_send_char(chr);
    delay_us(400);
    }
  delay_us(50);

  output_low(RS485_EN); // disable transmit
  comm_state = COM_IDLE;
  packet_idx = 0;
  flush_rx_buffer(1);
  locker_command_sent[door]++;
  }

//-----------------------------------------------------------------------------
void send_to_reader(u32 floor, u8 command)
  {
  send_to_lockers(floor+128, command, "");
  }

//-----------------------------------------------------------------------------
void send_locker_unlock_command(u32 door)
  {
  send_to_lockers(door, 'U', "");
  delay_ms(50);
  send_to_lockers(door, 'U', "");
  delay_ms(50);
  send_to_lockers(door, 'U', "");
  delay_ms(1);
  locker_unlocked[door] = 1;
  }

//-----------------------------------------------------------------------------
void send_locker_unlock_time(void)
  {
//  char buf[9];
//  sprintf(buf, " %d", doors[0].Unlock_time);
//  send_to_lockers(160, 'T', buf);
//  delay_ms(1);
  }

//----------------------------------------------------------------------------


u32 next_inbyte2;

u8 get_next2(void)
  {
  return COM2_buf[next_inbyte2++];
  }

void skip_space(void)
  {
  while (COM2_buf[next_inbyte2] && COM2_buf[next_inbyte2] == ' ')
    next_inbyte2++;
  }

u32 get_str_integer(void)
  {
  u32 acc = 0;
  skip_space();
  while (isdigit(COM2_buf[next_inbyte2]))
    {
    acc = acc * 10 + (COM2_buf[next_inbyte2++] - '0');
    }
  return acc;
  }

u32 get_hex(u8 *ptr)
 {
 u32 acc = 0;
 while (isxdigit(*ptr))
   {
   if (*ptr <= '9')
     acc = acc * 16 + (*ptr++ - '0');
   else
     acc = acc * 16 + (*ptr++ - 'A' + 10);
   }
 return acc;
 }

bool check_iox_checksum(void)
  {
  u32 idx, ccheck = 0, rcheck;
  idx = 0;
  while (COM2_buf[idx] != ';')
    {
    ccheck += COM2_buf[idx++];
    }
  idx++; // skip ;
  rcheck = get_hex(&COM2_buf[idx]);
  return (ccheck & 255) == (rcheck & 255);
  }

void process_iox16(void)
  {
  u32 STid_site;
  u32 STid_tag;
  u32 addr, index;
  u8 status, stat;
  u32 tag;
  if (COM2_buf[0] == '#')
    {
    if (check_iox_checksum() == 0)
      {
      return;
      }
    next_inbyte2 = 1;
    addr = get_str_integer();
    skip_space();
    status = get_next2();
    switch (status)
      {
      case 'T': // new tag
        skip_space();
        tag = 0;
        index = 0;
        while (1)
          {
          stat = get_next2();
          if (isxdigit(stat))
            {
            if (isdigit(stat))
              stat -= '0';
            else
              stat = (stat - 'A') + 10;
            tag <<= 4;
            tag |= stat;
            if ((++index & 1) == 0)
              {
              reader_data[0][index >> 1] = tag & 0xFF;
              }  
            }
          else
            {
            break;
            }
          }
        addr &= 127;

#if __STID_GOV_RDR__ == 1
        STid_tag = make32(reader_data[0][0], reader_data[0][1], reader_data[0][2], reader_data[0][3]);
        STid_tag <<= 1; // shift left
        STid_site = STid_tag >> 24; // isolate site code
        STid_tag = (STid_tag & 0x00FFFF00) >> 8;
        STid_tag += STid_site * 100000;
        tag = STid_tag;
#else    
        //        if (tag != previous_tag[addr])
        //          {
        tag = extract_wiegand_key(doors[0].First_digit, doors[0].Number_of_digits, &reader_data[0][1]);
#endif          
        process_key(addr, tag, 0);
        locker_tags[addr]++;
        //          }
        previous_tag[addr] = tag;
        tag_timeout[addr] = 10;
        break;
      case 'P': // present
        break;
      case 'S':
        skip_space();
        for (index = 0; index < 16; index++)
          {
          stat = get_next2();
          if (stat < 0x80)
            {
            stat -= 'A';
            locker_status[addr + index] = stat;
            }
          }
        break;
      }
    }
  }


void handle_IOX16_messages(void)
  {
  u8 chr;
  while (Subnet_char_ready())
    {
    if (IOX16_state == 0)
      {
      chr = GetCharFromSubnet();
      if (chr == X16PREAMB)
        {
        if (prev_char2 == X16PREAMB)
          {
          packet_idx2 = 0;
          COM2_buf[packet_idx2++] = X16PREAMB;
          IOX16_state = 1;
          }
        }
      prev_char2 = chr;
      }
    else if (IOX16_state == 1)
      {
      chr = GetCharFromSubnet();
      COM2_buf[packet_idx2++] = chr;
      if (packet_idx > 32)
        {
        IOX16_state = 0;
        }
      else if (chr == '\r')
        {
        COM2_buf[packet_idx2] = 0;
        process_iox16();
        IOX16_state = 0;
        }
      prev_char2 = chr;
      }
    else
      {
      IOX16_state = 0;
      }
    }
  if (Timer_10mS_Flags & TMR_10mS_Locker_Timeout)
    {
    Timer_10mS_Flags &= ~TMR_10mS_Locker_Timeout;
    
    if (tag_timeout[scan_reader_timeout])
      {
      if (!--tag_timeout[scan_reader_timeout])
        {
        previous_tag[scan_reader_timeout] = 0;
        }
      }
    if (++scan_reader_timeout > 64)
      {
      scan_reader_timeout = 0;
      }
    }
  }

