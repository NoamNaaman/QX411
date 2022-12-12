#include "main.h"
#include "setup.h"
#include "serial.h"


#define HOST_NO_COMM_TIMEOUT 16 // seconds

#define MAX_RETRIES 2

u16  latest_message_crc, previous_message_crc, prev_msg_timer, send_retries;
bool   GKP_wait_ack;
u16 pace_event_send;
u16 no_comm_timer;

u16  prev_messages[32];
u16 prev_sources[32], pmx;

u16 event_detected, ack_detected;
u16 delay_events = 0;

#define MAX_RESEND_EVENTS 64
u16  resend_events[MAX_RESEND_EVENTS];
u16  resevx = 0, resevc = 0, resevo = 0;

u32 next_inbyte, next_outbyte;

u32 ResetElevatorSystemCtr;

extern u32 elevator_system;

extern bool RS485_sending;
extern u32 seconds_from_reset;

bool process_message(void);
void put_event_record(u16 index, EVENT_RECORD *evt);
u32 RTC_compute_absolute_time(void);
void send_locker_unlock_command(u32 door);
void send_locker_unlock_time(void);
void enable_floor(u32 floor);
u32 RTC_compute_absolute_time(void);

void reset_lantronix(void)
  {
  output_drive(ETH_RST);
  output_low(ETH_RST);
  delay_ms(100);
  output_high(ETH_RST);
  delay_ms(100);
  }
/*
void test_ethernet(void)
  {
  u8 x;
  output_drive(ETH_RST);
  output_low(ETH_RST);
  delay_ms(100);
  output_high(ETH_RST);
  delay_ms(100);
  U4TXREG = 'X';
  delay_ms(400);
  x++;
  x++;
  }
*/
//------- COM1 hardware UART -----------------------------------------------
//=============================================================================
//-----------------------------------------------------------------------------
s32 msgcmp(void * s1,char *s2,u8 n)
  {
  char *su1, *su2;
  last_function = 1;
  for (su1=s1, su2=s2; 0<n; ++su1, ++su2, --n)
    {
     if(*su1!=*su2)
       {
       return ((*su1<*su2)?-1:+1);
       }
    }
  return 0;
  }

//-----------------------------------------------------------------------------
bool COM1_char_ready(void)
  {
//  return COM1_rcnt != 0;
  return get_comm_buffer_status(1);
  }

//-----------------------------------------------------------------------------
bool CommCharReady(void)
  {
  if (elevator_system)
    {
    return get_comm_buffer_status(2);
    }
  
  return get_comm_buffer_status(1) || get_comm_buffer_status(2);
  }

//-----------------------------------------------------------------------------
u8 CommGetChar(void)
  {
  if (!elevator_system) // regular system
    {              // both USARTs are enabled for PC comm
    if (get_comm_buffer_status(1))
      {
      return get_usart_byte(1);
      }
    else if (get_comm_buffer_status(2))
      {
      Ethernet_active = 12345;
      return get_usart_byte(2);
      }
    }
  else             // elevator system
    {             // only XPORT channel is used for PC
    if (get_comm_buffer_status(2))
      {
      Ethernet_active = 12345;
      return get_usart_byte(2);
      }
    }
  return 0;
  }

//=============================================================================
void transmit_char(u16 chr)
  {
  last_function = 2;
  ETHR_send_char(chr);
  if (!elevator_system)  
    {
    USART1->DR = chr; // send to RS485 with no delays since ETHR_send_char() already does
    }
  }

//=============================================================================
u16 COM1_get_chr(void)
  {
  u8 x;
  last_function = 3;
  x = get_usart_byte(1);
  return x & 255;
  }

//-----------------------------------------------------------------------------
bool COM1_send_packet(u8 dest, u8 source, u32 length)
  {
  u16 crc;
  u16 chr;//, echo;
  u8 *bp;
  bool fail;
  last_function = 4;
  comm_tbuf[0] = PREAMBLE0;                     // 0 preamble 2 bytes
  comm_tbuf[1] = PREAMBLE1;                     // 1
  comm_tbuf[2] = dest;                          // 2 send to destination
  comm_tbuf[3] = source + MyAddress;
  comm_tbuf[4] = 0;                             // 4 future use
  comm_tbuf[5] = 0;                             // 5

  comm_tbuf[6] = length - 7;                        // 6
  crc = CalculateCRC(comm_tbuf, length);

  comm_tbuf[length++] = make8(crc,0);                // insert crc at end of packet
  comm_tbuf[length++] = make8(crc,1);
  comm_tbuf[length++] = 0;

  COM1_rxi = COM1_rxo = COM1_rcnt = 0;
  fail = 0;

  while (1)
    {
    delay_us(200); // check for clear bus
    if (!COM1_rcnt)
      {
      break;
      }
    u32 idx = CommGetChar();
    }

//  output_high(X_RS485EN);
  
  if (!elevator_system) // regular system
    {
    output_high(RS485_EN); // enable RS485 transmit
    RS485_sending = 1;
    delay_us(20);
    }
  else // elevator system
    {
//    output_high(XPORT_EN);
    delay_us(10);
    }
  bp = comm_tbuf;
  transmit_char(0);
  delay_us(50);
  while (length--)
    {
    chr = *bp++;
    transmit_char(chr & 255);
    delay_us(30);
    }
  delay_us(10);
  if (!elevator_system)
    {
    output_low(RS485_EN); // disable transmit
    RS485_sending = 0;
    flush_rx_buffer(1);
    }

//  output_low(X_RS485EN);
  
//  if (elevator_system) // regular system
//    {
//    output_low(XPORT_EN);
//    }
  return !fail;
  }

//-----------------------------------------------------------------------------
void COM1_send_string(u8 *bp)
  {
  u8 chr;
//  u16 next;
 last_function = 5;
  if (!elevator_system)
    {
    output_high(RS485_EN); // enable transmit
    delay_us(100);
    }
  while (*bp)
    {
    chr = *bp++;
    transmit_char((u16)chr & 255);
    delay_us(300);
    }
  delay_us(50);
  if (!elevator_system)
    {
    output_low(RS485_EN); // disable transmit
    flush_rx_buffer(1);
    }
  }

//-----------------------------------------------------------------------------
void  COM1_send_ack(u8 source, u8 Destination)
  {
  u8 next, *bp, comm_tbuf[16];
  u16 length;
   last_function = 6;
  comm_tbuf[0] = PREAMBLE0;                     // 0 preamble 2 bytes
  comm_tbuf[1] = PREAMBLE1;                     // 1
  comm_tbuf[2] = Destination;                   // 2 send to EIM-1
  comm_tbuf[3] = source;                        // 3 from device 0

  comm_tbuf[4] = 0;                             // 4 future use
  comm_tbuf[5] = 0;                             // 5

  comm_tbuf[6] = 0xAC;                          // 6
  comm_tbuf[7] = 'K' ;                          // 7

  COM1_rxi = COM1_rxo = COM1_rcnt = 0;

  length = 9;

  if (Ethernet_active != 12345)
    {
    output_high(RS485_EN); // enable transmit
    delay_us(10);
    }
  bp = comm_tbuf;
  next = 0;
  RS485_sending = 1;
  while (length--)
    {
    transmit_char(next);
    next = *bp++; // buf[idx++];
    delay_us(30);
    }
  delay_us(10);
  output_low(RS485_EN); // disable transmit
  RS485_sending = 0;
  flush_rx_buffer(1);
  }

//-----------------------------------------------------------------------------
void COM1_init(void)
  {
 last_function = 7;
  flush_rx_buffer(1);
  output_low(RS485_EN); // disable transmit
  comm_timer = 0;
  packet_idx = 0;
  comm_state = 0;
  }

//--------------------------------------------------------------------------
bool check_packet_CRC(u8 *buf)
  {
  u16 rCRC, cCRC;
  u16 mlength;
  bool result;
   last_function = 8;
  mlength = (u16)buf[6] + 7;
  rCRC = make16(buf[mlength+1],buf[mlength]);
  cCRC = CalculateCRC(buf, mlength);
  latest_message_crc = rCRC;
  result = cCRC == rCRC;
  return result;
  }

//----------------------------------------------------------------------------
void init_comm(void)
  {
   last_function = 9;
  GKP_wait_ack = 0;
  COM1_init();
  send_retries = 0;
  prev_msg_timer = 0;
  }

//----------------------------------------------------------------------------
//u16 next_inbyte, next_outbyte;

//=============================================================================================
u16 get_next(void)
  {
   last_function = 10;
  return comm_buf[next_inbyte++];
  }

//=============================================================================================
u16 get_short(void)
  {
  u8 data;
   last_function = 11;
  data = get_next();
  return data;
  }

//=============================================================================================
u16 get_integer(void)
  {
  u16 data;
   last_function = 12;
  data = get_next();
  data |= ((u16)get_next() << 8);
  return data;
  }

//=============================================================================================
u32 get_long(void)
  {
  union {
        u32 data;
        u8 c[4];
  } u;
  u16 x;
   last_function = 13;
  for (x = 0; x < 4; x++)
    u.c[x] = get_next();
  return u.data;
  }

//=============================================================================================
void put_out_next(u8 dat)
  {
  comm_tbuf[next_outbyte++] = dat;
  }

//=============================================================================================
void put_short(u8 dat)
  {
  put_out_next(dat);
  }

//=============================================================================================
void put_integer(u16 dat)
  {
  put_out_next(make8(dat, 0));
  put_out_next(make8(dat, 1));
  }

//=============================================================================================
void put_long(u32 dat)
  {
  put_out_next(make8(dat, 0));
  put_out_next(make8(dat, 1));
  put_out_next(make8(dat, 2));
  put_out_next(make8(dat, 3));
  }

//=============================================================================================
void send_fire_alarm(u32 onoff)
  {
   last_function = 15;
  next_outbyte = 7;
  put_short(MSG_FIRE);
#ifndef NEW_FIRE_HANDLING
  put_short(255);
#else
  put_short(doors[0].DoorGroup);
#endif
  if (onoff)
    {
    onoff = 0x55;
    }
  put_short(onoff);
  COM1_send_packet(160, 0, next_outbyte);
  memcpy(comm_buf, comm_tbuf, sizeof(comm_buf));
  process_message();
  }

//=============================================================================================
void send_pc_wakeup(void)
  {
  last_function = 120;
  next_outbyte = 7;
  put_short(MSG_STST);                                   // 7         event message
  COM1_send_packet(0, 0, next_outbyte);
  }

//=============================================================================================
bool send_event(u16 index, EVENT_RECORD *evt)
  {
  DATE_TIME dt;
  bool success;
   last_function = 16;
  next_outbyte = 7;
  put_short(MSG_EVNT);                                   // 7         event message
  put_short(index & 255);                                // 8         event record index low
  put_short(index >> 8);                                 // 9         event record index high
  long_to_date(evt->timestamp, &dt);
  memcpy(comm_tbuf+next_outbyte, &dt, 6);                // 10 .. 15  date/time
  next_outbyte += 6;
  put_short(evt->event_type);                            // 16        event type
  put_long(evt->ID & 0x00FFFFFFL);                         // 17,18     ID
  put_short(doors[evt->source].DoorGroup);               // 19        door group
  put_long(evt->Code);                                   // 20 .. 23  tag code is applicable
  GKP_wait_ack = 1;
  success = COM1_send_packet(0, evt->source, next_outbyte);
  return success;
  }

//=============================================================================================
void ExecuteSpecialCommands(u32 code)
  {
  switch (code)
    {
    case 0x1111:  erase_keys_in_DB(); count_all_key = 0;  break;
    case 0x2222:  erase_events(); break;
    case 0x3333:  
      erase_keys_in_DB(); count_all_key = 0;
      erase_events();
      DB_erase_Al_wk_dy(); 
      break;
    case 0x4444:  break;
    case 0x5555:  break;
    case 0x9999:  
      NVIC_SystemReset();
      while (1);
      break;
    }
  }

//=============================================================================================
bool get_key_record(KEY_RECORD *kr, u32 *ID, u32 *kcode)
  {
  u32 bx, addr, count;
  static u32 l, user_id, key_code, pin_code;
  u16 doors;
  KEY_RECORD u;
   last_function = 17;
  user_id = get_long();
  u.ID = user_id;
  *ID = user_id;
  key_code = get_long();
  u.Code = key_code;
  *kcode = u.Code;

  pin_code = get_long();
  if (u.Code == 0xABCD1230 && (pin_code & 0xFFFF0000) == 0x12340000) // special codes?
    {
    ExecuteSpecialCommands(pin_code & 0xFFFF);
    return 0;
    }
  
  u.PIN = CalculateCRC((u8 *)&pin_code, 4); // compress 4 byte pin code to 2 bytes using CRC
  
  u.FmDate = get_integer();
  u.ToDate = get_integer();
  /*u.Ugroup = */get_short();
  u.al[0] = get_short();
  u.al[1] = get_short();
  u.Flags = get_integer();
  count = get_short();
  if (u.Flags & KFLAG_EntryLimit)
    {
    l = count;
    u.ID |= l << 24;
    }
  if (u.Flags & KFLAG_PersonalAL)   // save personal doors in FmDate field bits 14,15
    {
    addr = MyAddress - 1;
    bx = addr >> 3;
    doors = comm_buf[next_inbyte + bx];
    addr &= 7;
    doors <<= 14 - addr;
    u.FmDate |= doors;
    }
//  u.create_seq = global_setup.create_seq;
  memcpy(kr, &u, sizeof(u));
  return 1;
  }

//=============================================================================================
void send_status(u32 door)
  {
  u16 status;
  status = 0;
   last_function = 18;
  if (two_man_timer[door])
    {
    status |= 0x80;
    }
  if (unlock10[door])
    {
    status |= 0x40;
    }
  if (!door_closed[door])
    {
    status |= 8;
    }
  if (RTE_pressed[door])
    {
    status |= 2;
    }
  if (fire_condition[door])
    {
    status |= 16;
    }
  if (doors[door].DisableDoor == 0xDA)
    {
    status |= 32;
    }

  if (aux_timer[door])
    {
    status |= 0x100;
    }

  next_outbyte = 7;
  put_short(MSG_SSTT);                             // send status
  put_integer(status);                                // 8         event record index low
  put_long(keys_in_eeprom);
  put_integer(doors[door].Local_param[LPRM_TRAFFIC_COUNT]);
  put_short((u8)SW_VERSION);
  put_short(3);                             // this is a NEW DX2 controller
  put_integer(event_index);
  put_integer(event_not_sent);
  put_integer(event_next_send);
  COM1_send_packet(0, door, next_outbyte);
  }

//=============================================================================================
void send_weekzone_msg(void)
  {
  u16 ix;
   last_function = 19;
  next_outbyte = 7;
  put_short(MSG_STZN);
  put_short(1);
  put_short(0x7F);
  put_short(0xFF);
  for (ix = 0; ix < 4; ix++)
    {
    put_short(0);
    put_short(95);
    }
  COM1_send_packet(0, 0, next_outbyte);
  }

////----------------------------------------------------------------------------
//void send_key_record(KEY_RECORD *key_rec)
//  {
//   last_function = 20;
//  next_outbyte = 7;
//  put_short(MSG_SKEY);
//  put_long(key_rec->ID);
//  put_long(key_rec->Code);
//  put_long(0); // PIN code
//  put_integer(0);
//  put_integer(0x3FFF);
//  put_short(0); //ugroup
//  put_short(1);
//  put_short(0);
//  put_integer(0); // flags
//  COM1_send_packet(0, 0, next_outbyte);
//  }

//=============================================================================================
void get_door_record(u32 addressed_door)
  {
//        red_led(1); green_led(1);
//        delay_ms(50);
//        red_led(0); green_led(0);

  last_function = 22;
  get_short(); // skip door id
  doors[addressed_door].Unlock_time = get_integer();
  doors[addressed_door].DoorGroup = get_short();
  doors[addressed_door].ReaderTZ = get_short();
  doors[addressed_door].AutoTZ = get_short();
  doors[addressed_door].DisableTZ = get_short();
  doors[addressed_door].Flags = get_long();
  doors[addressed_door].Reader_type = get_short();
  doors[addressed_door].First_digit = get_short();  //  1st digit
  doors[addressed_door].Number_of_digits = get_short(); //  number of digits
  doors[addressed_door].AjarTime = get_short();
  doors[addressed_door].Alternate_mode = 0;
  if (doors[addressed_door].Reader_type & 128)
    {
    doors[addressed_door].Alternate_mode = 1;
    doors[addressed_door].Reader_type &= 15;
    }
  else
    {
    doors[addressed_door].Alternate_mode = 0;
    }
  allow_write = 2975;
  write_door_setup(addressed_door);
  reload_door_setup = 0;
  auto_unlocked[addressed_door] = 0;
  enable_first_user[addressed_door] = 0;
  if (unlock10[addressed_door])
    {
    unlock10[addressed_door] = 2;
    }
  if (addressed_door == 0)
    {
    if (test_door_flag(0, LFLAG_ELEVATOR_CONTROL))
      {
      elevator_system = 1;
      }
    else
      {
      elevator_system = 0;
      }
    }

  if (doors[addressed_door].Reader_type == RDR_WIEGAND)
    {
    WGND_init(addressed_door);
    }
//  generate_event(addressed_door, 0, 0, EVT_DoorSetupChanged);
  }

//=============================================================================================
bool message_not_processed(u8 source, u16 mCRC)
  {
  u16 ix;
  last_function = 23;
  for (ix = 0; ix < 32; ix++)
    {
    if (prev_sources[ix] == source && prev_messages[ix] == mCRC)
      {
      return 0;
      }
    }
  prev_messages[pmx & 31] = mCRC;
  prev_sources[pmx & 31] = source;
  if (++pmx >= 32)
    {
    pmx = 0;
    }
  return 1;
  }

//=============================================================================================
void process_incoming_event(u8 *comm_buf)
  {
  u16 mCRC, idx, event_type;
  u32 kcode, lidx;
  KEY_RECORD key;
  u8 dgrp, count;
  last_function = 24;
  bool apb_mode, interlock_mode, traffic_mode, update_apb;
//  ID = make16(comm_buf[20], comm_buf[19]);
  mCRC = make16(comm_buf[27], comm_buf[26]);
  update_apb = 1;
  event_type = comm_buf[16];
  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    if (MyAddress + idx == comm_buf[3]) // do not process message from this door as it was already processed BEFORE sending the event
      {
      continue;
      }
    dgrp = doors[idx].DoorGroup;
    if (dgrp && dgrp == comm_buf[21]) // is this the same door group as mine?
      {
      interlock_mode = test_door_flag(idx, LFLAG_INTERLOCK)  != 0;
      apb_mode       = test_door_flag(idx, (LFLAG_SOFT_APB|LFLAG_HARD_APB))   != 0;
      traffic_mode   = test_door_flag(idx, LFLAG_TRAFFIC_LT) != 0;
      if (apb_mode || interlock_mode || traffic_mode)
        {
        switch (event_type)
          {
          case EVT_remote_entry:
            if (traffic_mode)
              {
              lidx = 0;
              goto check_tr_entry;
              }
            break;
          case EVT_Valid_Entry:           //   1
            kcode = make32(comm_buf[25],comm_buf[24],comm_buf[23],comm_buf[22]);
            lidx = DB_search_key(kcode, &key);
            if (apb_mode && update_apb)
              {
              update_apb = 0;
              if (lidx)
                {
                set_apb_counter(lidx, APB_ENTRY_FLAG);
                }
              }

check_tr_entry:
            if (traffic_mode)
              {
              if (message_not_processed(comm_buf[3], mCRC))
                {
                doors[idx].Local_param[LPRM_TRAFFIC_COUNT]++;
                if (doors[idx].Local_param[LPRM_TRAFFIC_COUNT] >= doors[idx].Local_param[LPRM_TRAFFIC_THRESH])
                  {
                  operate_aux(idx, 0xFFFF); // turn red traffic light on
                  }
                }
              }
            
            if (lidx)
              {
              if (key.Flags & KFLAG_EntryLimit)
                {
                count = make8(key.ID, 3); // get counter
                if (count)
                  {
                  key.ID -= 0x01000000L; // decrement counter
                  DB_store_key(&key, lidx);
                  }
                }
              }
            goto check_unlock;

          case EVT_remote_exit:
            if (traffic_mode)
              {
              goto check_tr_exit;
              }
            break;
          case EVT_Valid_Exit:            //   2
            if (apb_mode && update_apb)
              {
              update_apb = 0;
              kcode = make32(comm_buf[25],comm_buf[24],comm_buf[23],comm_buf[22]);
              lidx = DB_search_key(kcode, &key);
              if (lidx)
                {
                set_apb_counter(lidx, APB_EXIT_FLAG);
                }
              }
            if (interlock_mode)
              {
              interlock_disable_unlock[idx] |= 2;
              }

check_tr_exit:
            if (traffic_mode)
              if (message_not_processed(comm_buf[3], mCRC))
                {
                if (doors[idx].Local_param[LPRM_TRAFFIC_COUNT])
                  {
                  doors[idx].Local_param[LPRM_TRAFFIC_COUNT]--;
                  operate_aux(idx, 0); // turn red traffic light off
                  }
                }
            goto check_unlock;

          case EVT_Remote_open: 
          case EVT_Request_to_Exit:
check_unlock:
            if (interlock_mode)
              {
              interlock_disable_unlock[idx] |= 2;
              }
            break;
          case EVT_Door_forced_open: case EVT_door_opened:
            if (interlock_mode)
              {
              interlock_disable_unlock[idx] |= 1;
              }
            break;
          case EVT_door_closed:
            if (interlock_mode)
              {
              if (interlock_disable_unlock[idx])
                {
                interlock_disable_unlock[idx] &= 0xFFFE;
                }
              }
            break;
          case EVT_relocked:
            if (interlock_mode)
              {
              if (interlock_disable_unlock[idx])
                {
                interlock_disable_unlock[idx] &= 0xFFFD;
                }
              }
            break;
          }
        }
      }
    }
  }

//=============================================================================================
void get_access_level(void)
  {
  u16 aln, index;
  AL_RECORD al;
  last_function = 25;
  aln = get_short();
  al.weekzone[0] = get_short();
  al.weekzone[1] = get_short();

  for (index = 0; index < 16; index++)
    {
    al.ValidDoor[0][index] = get_short();
    }
  for (index = 0; index < 16; index++)
    {
    al.ValidDoor[1][index] = get_short();
    }

  DB_store_access_level(aln, &al);

  MyAddress = get_address();
  }


//----------------------------------------------------------------------------
void get_time_zone(void)
  {
  u8 *tzp;
  u16 idx, bx;
  WZ_RECORD   tz;
  last_function = 26;
  idx = get_short();
  tzp = &tz.DOW[0];
  for (bx = 0; bx < 8; bx++)
    {
    *tzp++ = get_short();
    }
  DB_store_weekzone(idx, &tz);
  }

//----------------------------------------------------------------------------
void get_day_zone(void)
  {
  u8 *tzp;
  u16 idx, bx;
  DZ_RECORD   dz;
  last_function = 27;
  idx = get_short();
  tzp = &dz.range[0][0];
  for (bx = 0; bx < 8; bx++)
    {
    *tzp++ = get_short();
    }
  DB_store_dayzone(idx, &dz);
  }


//----------------------------------------------------------------------------
void remote_auxiliary(u16 addressed_door)
  {
  u16 bx;
  static u16 prev_door, prev_op;
  static u32 prev_time;
  u32 current_time = RTC_compute_absolute_time();
  last_function = 28;
  bx = get_short(); // get operation
  if (addressed_door == prev_door && prev_op == bx && current_time - prev_time < 6)
    {
    return;
    }
  
  prev_time = current_time;
  prev_door = addressed_door;
  prev_op = bx;

  generate_event(addressed_door, 0, 0, EVT_remote_auxiliary);

  if (!bx)
    {
    operate_aux(addressed_door, 0);
    }
  else
    {
    if (bx == 255)
      {
      bx = 0xFFFF;
      }
    aux_timer[addressed_door] = bx * 10;
    operate_aux(addressed_door, 1);
    }
  }

//----------------------------------------------------------------------------
void enable_unlock(u16 ix)
  {
  last_function = 29;
  doors[ix].DisableDoor = 0;
  allow_write = 2975;
  update_internal_setup[ix] = 1;
  generate_event(ix, 0,0, EVT_EnDoor);
  auto_unlocked[ix] = 0;
  write_door_setup(ix);
  }

//----------------------------------------------------------------------------
void disable_unlock(u16 ix)
  {
  last_function = 30;
  doors[ix].DisableDoor = 0xDA;
  allow_write = 2975;
  update_internal_setup[ix] = 1;
  write_door_setup(ix);
  generate_event(ix, 0, 0, EVT_DisDoor);
  auto_unlocked[ix] = 0;
  if (unlock10[ix])
    {
    unlock10[ix] = 5;
    }
  }

//----------------------------------------------------------------------------
void energize_single_floor_relay(u32 floor)
  {
  if (floor < 8)
    {
    enable_floor(floor);
    }
  else
    {
    send_locker_unlock_time();
//    send_locker_unlock_command(floor + 1);
    send_locker_unlock_command(floor - 8 + 1);
    }
  }

//----------------------------------------------------------------------------
void remote_unlock(u16 addressed_door)
  {
  u16 bx;
  static u16 prev_door, prev_op;
  static u32 prev_time;
  u32 current_time = RTC_compute_absolute_time();
  last_function = 31;
  
  if (elevator_system)
    {
    energize_single_floor_relay(addressed_door);
    generate_event(addressed_door, 0, 0, EVT_Remote_open);
    return;
    }

  if (fire_condition[addressed_door])
    {
    return;
    }

  bx = get_short(); // get operation
  if (addressed_door == prev_door && prev_op == bx && current_time - prev_time < 6)
    {
    return;
    }
  
  prev_time = current_time;
  prev_door = addressed_door;
  prev_op = bx;
  
//  if (!auto_unlocked[addressed_door])
    {
    if (!bx)
      {
      relock(addressed_door);
      }
    else
      {
      if (bx == 255)
        {
        unlock_door(addressed_door);
        unlock10[addressed_door] = 65535;
        generate_event(addressed_door, 0, 0, EVT_RemoteLeaveDoorOpen);
        }
      else if (bx == 254)
        {
        if (enable_first_user[addressed_door])
          {
          unlock_door(addressed_door);
          enable_first_user[addressed_door] = 0;
          auto_unlocked[addressed_door] = 1;
          generate_event(addressed_door, 0, 0, EVT_Auto_door_open);
          }
        }
      else if (bx == 252)
        {
        enable_unlock(addressed_door);
        }
      else if (bx == 253)
        {
        disable_unlock(addressed_door);
        }
      else
        {
        if (unlock10[addressed_door] == 0)
          {
          generate_event(addressed_door, 0, 0, EVT_Remote_open);
          }
        unlock_door(addressed_door);
        unlock10[addressed_door] = bx * 10;
        }
      }
    }
  }

//----------------------------------------------------------------------------
void handle_fire(void)
  {
  u16 ix, al, onoff;
  u32 albyteX = (MyAddress-1) / 8;
  u32 mask = 1 << ((MyAddress-1) & 7);
  union {
        AL_RECORD   al;
        WZ_RECORD   tz;
        DZ_RECORD   dz;
        KEY_RECORD  ky;
        EVENT_RECORD ev;
        } u;
  last_function = 32;
  al = get_next();
  if (al)
    {
    DB_load_access_level(al, &u.al);
    }
  onoff = get_short();
  for (ix = 0; ix < MAX_DOORS; ix++)
    {
    if (al) // is this command using an access level?
      {
      if (u.al.ValidDoor[0][albyteX] & mask) // check if door is valid selection
        {
        goto fire_mode;
        }
      }
    else if (doors[ix].Flags & LFLAG_FIRE_DOOR)
      {
fire_mode:
      if (onoff == 0x55)
        {
        unlock_door(ix);
        unlock10[ix] = 0xFFFF; // unlock
        fire_condition[ix] = 1;
        }
      else
        {
        fire_condition[ix] = 0;
        if (unlock10[ix])
          {
          unlock10[ix] = 10; // relock
          }
        }
      }
    mask <<= 1;
    if (mask == 0x100)
      {
      mask = 1;
      albyteX++;
      }
    }
  }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void disable_door_unlock(void)
  {
  u32 al, mask;
  u32 ix, albyteX;

  union {
        AL_RECORD   al;
        WZ_RECORD   tz;
        DZ_RECORD   dz;
        KEY_RECORD  ky;
        EVENT_RECORD ev;
        } u;

  last_function = 33;
  
  albyteX = (MyAddress-1) / 8;
//  mask = 1 << ((MyAddress-1) & 7);
  
  al = get_next();
  if (al) // is this command using an access level?
    {
    DB_load_access_level(al, &u.al);
    }
  for (ix = 0; ix < MAX_DOORS; ix++)
    {
    if (al) // is this command using an access level?
      {
      mask = 1 << ix;
      if (u.al.ValidDoor[0][albyteX] & mask) // check if door is valid selection
        {
        disable_unlock(ix);
        }
      }
    else if (doors[ix].Flags & LFLAG_DIS_UNLOCK)
      {
      disable_unlock(ix);
      }
    }
  }

//----------------------------------------------------------------------------
void enable_door_unlock(void)
  {
  u32 al, mask;
  u32 ix, albyteX;

  union {
        AL_RECORD   al;
        WZ_RECORD   tz;
        DZ_RECORD   dz;
        KEY_RECORD  ky;
        EVENT_RECORD ev;
        } u;

  last_function = 33;
  
  albyteX = (MyAddress-1) / 8;
//  mask = 1 << ((MyAddress-1) & 7);
  
  al = get_next();
  if (al) // is this command using an access level?
    {
    DB_load_access_level(al, &u.al);
    }
  for (ix = 0; ix < MAX_DOORS; ix++)
    {
    if (al) // is this command using an access level?
      {
      mask = 1 << ix;
      if (u.al.ValidDoor[0][albyteX] & mask) // check if door is valid selection
        {
        enable_unlock(ix);
        }
      }
    else if (doors[ix].Flags & LFLAG_DIS_UNLOCK)
      {
      enable_unlock(ix);
      }
    }
  }

//----------------------------------------------------------------------------
void store_key_record(KEY_RECORD *user_rec)
  {
  u32 lidx, kcode;
  bool exists;
  last_function = 34;
  kcode = user_rec->Code;
  lidx = DB_search_key(kcode, user_rec);
  exists = lidx != 0;
  if (!exists)
    {
    lidx = DB_search_empty_slot(kcode);
    }
  if (lidx)
    {
    DB_store_key(user_rec, lidx);
    }
  }

//----------------------------------------------------------------------------
bool process_message(void)
  {
  u32 index, ID;
  u32 kcode, lidx;
  bool exists, command;
  bool my_addr_range;
  u16 addressed_door;
  u16  bx, ix, val, my_addr, msg_type;
  DATE_TIME date_time;
  union {
        AL_RECORD   al;
        WZ_RECORD   tz;
        DZ_RECORD   dz;
        KEY_RECORD  ky;
        EVENT_RECORD ev;
        } u;
  KEY_RECORD user_rec;
  last_function = 35;
  command = 0;
  next_inbyte = 7; // get data from byte 7
  my_addr = MyAddress;
  addressed_door = comm_buf[2] - my_addr;
  if (elevator_system)
    {
    u32 floors = doors[0].Local_param[LPRM_ELEVATOR_FLOORS];
    if (floors == 0)
      {
      floors = 64;
      }
    my_addr_range = comm_buf[2] >= my_addr && comm_buf[2] < my_addr + floors;
    }
  else
    {
    my_addr_range = comm_buf[2] >= my_addr && comm_buf[2] < my_addr + MAX_DOORS;
    }
  if (my_addr_range || comm_buf[2] == 160 || comm_buf[7] == MSG_EVNT)
    {
    if (my_addr_range)
      {
//      if (!elevator_system)
        {
        COM1_send_ack(comm_buf[2], comm_buf[3]);
        }
      if (latest_message_crc == previous_message_crc && prev_msg_timer)
        {
        return 0;
        }
      prev_msg_timer = 10;
      }
    else
      {
      previous_message_crc = 0;
      prev_msg_timer = 0;
      }
    msg_type = get_next();

    if (msg_type != MSG_SDTM)
      {
      __NOP();
      }
    
    
    switch (msg_type)
      {
      case MSG_RQST:
        if (addressed_door < MAX_DOORS)
          {
          send_status(addressed_door);
          }
        break;
      case MSG_ENUL:
        enable_door_unlock();
        break;
      case MSG_DSUL:
        disable_door_unlock();
        break;
      case MSG_FIRE:
        handle_fire();
        break;
      case MSG_SKEY:  //  1 set key record
        command = 1;
        if (!get_key_record(&u.ky, &ID, &kcode))
          {
          break;
          }
//search_key:
        recv_keys++;
        memcpy(&post_key, &u.ky, sizeof(u.ky));
        lidx = DB_search_key(kcode, &user_rec);
        exists = lidx != 0;
        if (!exists)
          {
          lidx = DB_search_empty_slot(kcode);
          }
        if (lidx)
          {
          DB_store_key(&u.ky, lidx);
          if (!exists)
            {
            keys_in_eeprom++;
            }
          }
        break;
      case MSG_DKEY: // delete key record
        command = 1;
        get_long(); // skip user ID
        kcode = get_long();
        lidx = DB_search_key(kcode, &u.ky);
        if (lidx)
          {                               // fill with zeros to mark that there maybe other keys further down the
          DB_delete_key(lidx);            // hash search path
          if (keys_in_eeprom)
            {
            keys_in_eeprom--;
            }
          }
        break;
      case MSG_ERAU:  //  4 erase all users
        if (get_short() == 0x27)
          {
          if (get_short() == 0x95)
            {
            erase_keys_in_DB();
            keys_in_eeprom = 0;
            }
          }
        break;
      case MSG_ERAL: // erase all keys
        if (get_short() == 'E')
          {
          if (get_short() == 'R')
            {
            if (get_short() == 'A')
              {
              if (get_short() == 'L')
                {
                erase_keys_in_DB();
                generate_event(addressed_door, 0, 0, EVT_erased_all_keys);
                }
              }
            }
          }
        break;
      case MSG_SDTM:  //  7 set date time
        memcpy(&date_time, comm_buf+next_inbyte, sizeof(DATE_TIME));
        for (u32 loop = 0; loop < 4; loop++)
          {
          RTC_load_date_time();
          if (date_time.day != sys_date_time.day ||
              date_time.hour !=  sys_date_time.hour ||
//              date_time.minute != sys_date_time.minute ||
              date_time.month != sys_date_time.month ||
              date_time.year != sys_date_time.year)
            {
            RTC_store_date_time(&date_time);
            delay_us(40000);
            }
          else
            {
            break;
            }
          }
        break;
      case MSG_ERAV:  // 10 erase events
        if (get_short() == 0x27)
          {
          if (get_short() == 0x95)
            {
            erase_events();
            }
          }
        break;
      case MSG_LPRM:  // set local parameter
        ix = get_short();     // get door group
        if (addressed_door == 160)
          {
          break;
          }
        if (!ix || ix == doors[addressed_door].DoorGroup)
          {
          ix = get_short() % LOCAL_PARAMETERS; // get param #
          val = get_integer();  // get value
          doors[addressed_door].Local_param[ix] = val;
          allow_write = 2975;
          update_internal_setup[addressed_door] = 1;
          write_door_setup(addressed_door);
          }
        break;
      case MSG_SALV:  // 16 set access level
        command = 1;
//        disable_interrupts(intr_global);
        get_access_level();
//        enable_interrupts(intr_global);
        break;
      case MSG_STZN:  // set time zone
        command = 1;
        get_time_zone();
        break;
      case MSG_SDZN:  // set day zone
        command = 1;
        get_day_zone();
        break;
      case MSG_SDRR:  // set door record
        get_door_record(addressed_door);
        command = 1;
        break;
      case MSG_UNLK:  // 22 remote unlock
        remote_unlock(addressed_door);
        break;
      case MSG_AUXR:  // 22 remote auxiliary
        remote_auxiliary(addressed_door);
        break;
      case MSG_REVT:  // 25 request event record
        index = make16(comm_buf[7], comm_buf[6]);

        if (index == 0xFFFF)
          {
          if ((index = event_index - 1) == 0xFFFF)
            {
            index = MAX_EVENTS - 1;
            }
          }
        get_event_record(index, &u.ev);
        send_event(index, &u.ev);
        break;
      case MSG_SMHL: // <Month><reserved><Days>
        command = 1;
        bx = get_short(); // get month #
        if (bx  && bx < 13)
          {
          get_short(); // get rid of future option byte
          DB_store_holidays(bx-1, comm_buf+next_inbyte);
          if (bx == sys_date_time.month)
            {
            memcpy(Month_special_days, comm_buf+next_inbyte, 31);
            }
          }
        break;
      case MSG_NKEY: // key not found
//        lidx++;//debug
        break;
      case MSG_EVNT: // event from another controller
        if (comm_buf[21]) // door group Not zero?
          {
          process_incoming_event(comm_buf);
          }
        event_detected = 1;
        break;
      }
    }
  else
    {
    previous_message_crc = 0;
    }
  previous_message_crc = latest_message_crc;
  return command;
  }

//----------------------------------------------------------------------------
void  mark_event_as_sent(u32 event_next_send)
  {
  EVENT_RECORD evt;
  
//  return;
  
  get_event_record(event_next_send, &evt);
  evt.ID |= 0x80000000;
  put_event_record(event_next_send, &evt);
  }

//----------------------------------------------------------------------------
void move_to_next_event(void)
  {
  EVENT_RECORD evt;
  send_retries = 0;
  get_event_record(event_next_send, &evt);
  if ((evt.ID & 0x80000000) == 0) // this event is assumed to have been received by host?
    {
    mark_event_as_sent(event_next_send);
    }
  event_not_sent--;
  if (++event_next_send >= MAX_EVENTS)
    {
    event_next_send = 0;
    }
  }

//----------------------------------------------------------------------------
void GKP_comm_handler(void)
  {
  u16 chr;
  u16 pace, my_addr;
  EVENT_RECORD evt;
  last_function = 37;
  if (!CommCharReady())
    {
    if ((Timer_1sec_Flags & Tmr_1sec_Elev_Reset))
      {
      Timer_1sec_Flags  &= ~(Tmr_1sec_Elev_Reset);
      if (elevator_system)
        {
        if (++ResetElevatorSystemCtr >= 3600 * 4) // 4 hours passed?
          {
          NVIC_SystemReset();
          }
        }
      }
    
    if (prev_msg_timer)
    if ((Timer_10mS_Flags & Tmr_10mS_COMM_TIMER))
      {
      Timer_10mS_Flags  &= ~(Tmr_10mS_COMM_TIMER);
      prev_msg_timer--;
      }
    
    if (Timer_1mS_Flags & Tmr_1mS_COMM_PACING) // check timeouts 1000 per second
      {
      Timer_1mS_Flags &= ~Tmr_1mS_COMM_PACING;
      if (packet_idx && comm_timer > 50)
        {
        COM1_init();
        }
      else if (ETH_detected && seconds_from_reset > 12)
        {
        if (event_not_sent)
          {
          if ((Timer_1mS_Flags & Tmr_1mS_SEND_EVENT))
            {
            Timer_1mS_Flags &= ~(Tmr_1mS_SEND_EVENT);
            if (delay_events)
              {
              delay_events--;
              }
            my_addr = (u16)MyAddress;
            if (Ethernet_active == 12345)
              {
              pace = 25;
              }
            else
              {
              pace = delay_events * 2 + (u16)200 + my_addr + ((my_addr % 7) + (u16)send_retries) * 6;
              }
            if (++pace_event_send < pace)
              {
              return;
              }
            COM1_init();
            pace_event_send = 0;
            no_comm_timer = 0;
            get_event_record(event_next_send, &evt);
            if (evt.ID & 0x80000000) // this event is assumed to have been received by host?
              {
              move_to_next_event();
              }
            else if (send_event(event_next_send, &evt))
              {
              send_retries++;
              if (send_retries == 1)
                {
                process_incoming_event(comm_tbuf); // update other doors on this controller
                }
              u32 retries = MAX_RETRIES;
              if (!Ethernet_active)
                {
                retries += 2;
                }
              if (send_retries >= retries)
                {
                move_to_next_event();
                }
              }
            }
          }
        }
      }
    return;
    }
  while (CommCharReady())
    {
    comm_timer = 0;
/*
    if (MyAddress == 0xC9)
      {
      transmit_char(CommGetChar()+1);
      continue;
      }
*/

    switch(comm_state)
      {
      case COM_IDLE:
        if (CommGetChar() == PREAMBLE0) // start of packet?
          {
          comm_buf[packet_idx++] = PREAMBLE0;
          comm_state = COM_RECD;
          }
        break;
      case COM_RECD:
        chr = CommGetChar();
        if (packet_idx == 1)
          {
          if (chr == PREAMBLE1)
            {
            comm_buf[packet_idx++] = PREAMBLE1;
            }
          else
            {
            COM1_init();
            }
          }
        else
          {
          if (packet_idx > COM1_TX_LEN - 2)
            {
            COM1_init();
            }
          else
            {
            comm_buf[packet_idx++] = chr;
            if (packet_idx == 7)
              {
              if (chr == 0xAC) // is this an ACK?
                {
                ack_detected = 1;
                comm_state = COM_RACK;
                }
              break;
              }
            if (packet_idx > 6 && packet_idx == comm_buf[6] + 9) // message + header + crc
              {
//              disable_interrupts(intr_global);
//              if (comm_buf[7] != MSG_SDTM)
//                {
//                chr = 5;
//                }
              if (check_packet_CRC(comm_buf))
                {
                if (comm_buf[3] == 0) // message from ETH?
                  {
                  ETH_detected = HOST_NO_COMM_TIMEOUT;
                  no_comm_timer = 0;
                  }
                if (process_message()) // is this a download command
                  {
                  pace_event_send = 0; // YES. delay transmission of events
                  delay_events = 20;
                  }
                }
//              enable_interrupts(intr_global);
              COM1_init();
              }
            }
          }
        break;
      case COM_RACK:
        chr = CommGetChar();
        if (comm_buf[3] == 0)
          {
          ETH_detected = HOST_NO_COMM_TIMEOUT;
          no_comm_timer = 0;
          if (chr == 'K') // ack from ETH?
            {
            if (comm_buf[2] >= MyAddress && comm_buf[2] < MyAddress + MAX_DOORS)
              {
              if (event_not_sent)
                {
                send_retries = 0;
                event_not_sent--;
                check_event_index_validity();
                mark_event_as_sent(event_next_send);
                if ((event_next_send & 3) == 3) // V25
                  {
                  write_ext_eeprom(ADDR_EVTPTR, (u8 *)&event_next_send, 2);
                  }
                if (++event_next_send >= MAX_EVENTS)
                  {
                  event_next_send = 0;
                  }
                }
              }
            }
          COM1_init();
          delay_ms(2);
          }
        break;
      }
    }
  }

