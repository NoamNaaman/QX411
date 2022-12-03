#include "main.h"

#include "setup.h"
#include "serial.h"
#include "stdio.h"

u32 locker_unlocked[64];
u32 locker_number;

//-----------------------------------------------------------------------------
void add_digit_to_lock_number(u32 digit)
  {
  locker_number = locker_number * 10 + digit;
  }

//-----------------------------------------------------------------------------
void init_locker_display(void)
  {
  locker_number = 0;
  }

//-----------------------------------------------------------------------------
void display_invalid_locker(void)
  {
  }

//-----------------------------------------------------------------------------
void display_locker_open(void)
  {
  }

//-----------------------------------------------------------------------------
void process_locker_number(void)
  {
  process_key(0, locker_number, 1);
  }

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
  delay_ms(1);
  locker_unlocked[door] = 1;
  }

//-----------------------------------------------------------------------------
void send_locker_unlock_time(void)
  {
  char buf[9];
  sprintf(buf, " %d", doors[0].Unlock_time);
  send_to_lockers(160, 'T', buf);
  delay_ms(1);
  }

//----------------------------------------------------------------------------

#if 0

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

check_iox_checksum(void)
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
  u32 addr, index;
  u8 status, stat, reader_data[8];
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
              reader_data[index >> 1] = tag & 0xFF;
              }  
            }
          else
            {
            break;
            }
          }
        addr &= 127;
//        if (tag != previous_tag[addr])
          {
          tag = extract_wiegand_key(doors[0].first_digit, doors[0].number_of_digits, &reader_data[1]);
          process_key(addr, tag, 0);
          }
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
  while (COM2_rcnt)
    {
    if (IOX16_state == 0)
      {
      chr = COM2_get_chr();
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
      chr = COM2_get_chr();
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
  if (TMR_10MS_CHECK_TIMEOUT)
    {
    TMR_10MS_CHECK_TIMEOUT = 0;
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
#endif