#include "main.h"

#include "setup.h"
#include "serial.h"
#include "stdio.h"

u32 locker_unlocked[64];
u32 locker_number, locker_digitX, locker_display_timer;


void put_string(u32 x, u32 y, u32 font_size, char *string);
void clear_screen(void);



//----------------------------------------------------------------------------
void init_locker_disp_timer(void)
  {
  locker_display_timer = 100; // 10 seconds
  }

//-----------------------------------------------------------------------------
void add_digit_to_lock_number(u32 digit)
  {
  char buf[4];
  buf[0] = digit + '0';
  buf[1] = 0;
  locker_number = locker_number * 10 + digit;
  put_string(60 + locker_digitX++ * 10, 0, 18, buf);
  
  }

//-----------------------------------------------------------------------------
void init_locker_display(void)
  {
  locker_number = 0;
  locker_digitX = 0;
  clear_screen();
  delay_ms(10);
  }

//-----------------------------------------------------------------------------
void display_invalid_locker(void)
  {
  clear_screen();
  put_string(0, 0, 18, "Invalid locker");
  init_locker_disp_timer();
  }

//-----------------------------------------------------------------------------
void display_present_tag(void)
  {
  clear_screen();
  put_string(0, 0, 18, "Present tag");
  }

//-----------------------------------------------------------------------------
void display_locker_open(void)
  {
  clear_screen();
  put_string(0, 0, 18, "Locker open");
  init_locker_disp_timer();
  }

//-----------------------------------------------------------------------------
void display_locker_query(void)
  {
  clear_screen();
  put_string(0, 0, 18, "Lock#:");
  init_locker_disp_timer();
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
void manage_locker_display(void)
  {
  if (locker_display_timer)
    {
    if ((Timer_100mS_Flags & Tmr_100mS_LOCKER_DSP) != 0)
      {
      Timer_100mS_Flags &= Tmr_100mS_LOCKER_DSP;
      if (--locker_display_timer == 0)
        {
        init_locker_display();
        display_present_tag();
        }
      }
    }
  }
