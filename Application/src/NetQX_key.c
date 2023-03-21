#include "main.h"

#include "setup.h"

bool valid_door_found;
u32 latest_key_data, key_checked;

u32 key_delay_timer[64];

KEY_RECORD key_record[64];
KEY_RECORD first_key[64];
u32 key_code[64];

u32 people_inside[MAX_DOORS]; // for dual presence feature
u32 first_person_timer[MAX_DOORS]; // for dual presence feature

u32 first_in_the_morning_occur[MAX_DOORS];
u32 first_in_the_morning_timer[MAX_DOORS];
bool reset_first_in_the_morning = true;

s32 movement_timer[MAX_DOORS];

u32 door_opened[64];

extern u32 elevator_system, WiegandReader2[];

void send_locker_unlock_command(u32 door);
void operate_relay(u32 relay, u32 onoff);
void send_locker_unlock_time(void);
bool Reader2IsSource(u32 door);
void ClearReader2(u32 door);
void send_locker_unlock_command(u32 door);

//--------------------------------------------------------------------------
bool dual_presence_feature_enabled(u32 source)
  {
  if (test_door_flag(source, LFLAG_2RDR_SINGLE_DOOR) &&    // apply input from two reader to a single door
      test_door_flag(source, LFLAG_DUAL_PRESENCE))       // room behind door with entry/exit readers requires at least two persons inside. if not, alarm
    {
    return true;
    }
  return false;
  }

//--------------------------------------------------------------------------
void handle_invalid_door(u32 source)
  {
  two_man_timer[source] = 0;
  two_man_count[source] = 0;
  generate_event(source, key_record[source].ID, key_code[source], EVT_Invalid_door);
  if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
    {
    set_buzzer(source, BZMODE_4_BEEPS);
    if (rdr4_active)
      {
      rdr4_red(source);
      delay_ms(50);
      rdr4_yellow(source);
      }
    }
  }

//--------------------------------------------------------------------------
//from/to time check
bool check_time_zone(u16 tzn)
  {
  WZ_RECORD tz;
  DZ_RECORD dz;
  u8 minute, special;
  u8 from, to;
  u16 tzx, dzx;
  
  minute = ((u16)sys_date_time.hour * 60 + (u16)sys_date_time.minute) / 15;
  
  DB_load_weekzone(tzn, &tz);
  special = Month_special_days[sys_date_time.day-1];
  if (special)
    {
    if (tz.SpecialDays == 0)
      {
      return 0;
      }
    dzx = tz.SpecialDays;
    }
  else
    {
    dzx = tz.DOW[day_of_week];
    }
  
//  if (!input(DISABLE_KEY_TIME))
//    {
//    return 1;
//    }
  
  DB_load_dayzone(dzx, &dz);
  for (tzx = 0; tzx < 4; tzx++)
    {
    from = dz.range[tzx][0];
    to   = dz.range[tzx][1];
    if (to < 96 && to >= from)
      {
      if (minute <= to && minute >=  from)
        {
        return 1;
        }
      }
    }
  return 0;
  }

//--------------------------------------------------------------------------
void enable_floor(u32 floor)
  {
  if (floor >= 4) // is this an aux relay?
    {
    operate_aux(floor - 4, doors[0].Unlock_time);
    }
  else
    {
    operate_lock(floor, 1);
    unlock10[floor] = doors[0].Unlock_time;
    }
  }


//--------------------------------------------------------------------------
void energize_all_elevator_relays(void)
  {
  u32 index;
  for (index = 0; index < 8; index++)
    {
    enable_floor(index);
    }
  for (u32 floor = 1; floor  <= doors[0].Local_param[LPRM_ELEVATOR_FLOORS] - 8; floor++)
    {
    send_locker_unlock_command(floor);
    }
  }

//--------------------------------------------------------------------------
void energize_elevator_relays(u32 aln)
  {
  AL_RECORD al;
  u32 index;
  u8 mask, byteX, byteD;
  send_locker_unlock_time();
  DB_load_access_level(aln, &al);
  if (al.weekzone[0])
    {
    if (check_time_zone(al.weekzone[0] == 0))
      {
      return;
      }
    }
  
  for (byteX = 0, index = 0; index < 128; byteX++)
    {
    byteD = al.ValidDoor[0][byteX];
    for (mask = 1; mask != 0; mask <<= 1, index++)
      {
      if ((byteD & mask) != 0)
        {
        send_locker_unlock_command(index);
//        return;
        }
      }
    }
  
//  u32 floor_cnt = doors[0].Local_param[7];
//  for (index = 0; index < 16; index++)
//    {
//    for (u32 floor = 1, mask = 1; floor < 9; mask <<= 1, floor++)
//      {
//      if ((floors & mask) != 0)
//        {
//        send_locker_unlock_command((index) * 8 + floor);
//        }
//      if (--floor_cnt == 0)
//        {
//        return;
//        }
//      }
//    }
  }

//--------------------------------------------------------------------------
bool check_access_level(u32 door, u32 aln)
  {
  AL_RECORD al;
  u32 mask, albyteX, nibbleX;
  u32 idx;
  
  if (!aln)
    {
    return 0;
    }
  door--; // zero based
  albyteX = (door) >> 3;
  mask = 1 << (door & 7);
  DB_load_access_level(aln, &al);
  for (idx = 0; idx < 2; idx++)
    {
    if (al.ValidDoor[idx][albyteX] & mask) // check if door is valid selection
      {
      valid_door_found = 1;
      if (check_time_zone(al.weekzone[idx])) //from/to time check
        {
        door_opened[door]++;
        return 1;
        }
      }
    }
  return 0;
  }

//--------------------------------------------------------------------------
bool allowed_to_open(u8 source, KEY_RECORD *key_rec)
  {
  u16 today, from, to;
  if (key_rec->Flags & KFLAG_Master)  // if this is a master tag, do not limit in any way
    {
    return 1;
    }
  
  if ((key_rec->Flags & (KFLAG_NoEntry | KFLAG_NoExit)) == (KFLAG_NoEntry | KFLAG_NoExit)) // if user is barred from BOTH entry
    {                                                                                     // and exit then this user is INACTIVE
    generate_event(source, key_rec->ID, key_rec->Code, EVT_Key_inactive);
    if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
      {
      set_buzzer(source, BZMODE_4_BEEPS);
      }
    return 0;
    }
  else
    {
    if ((test_door_flag(source, LFLAG_EXIT) && (key_rec->Flags &KFLAG_NoExit)) ||
        (!test_door_flag(source, LFLAG_EXIT) && (key_rec->Flags & KFLAG_NoEntry)))
      {
      generate_event(source, key_rec->ID, key_rec->Code, EVT_Unlock_not_permitted);
      if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
        {
        set_buzzer(source, BZMODE_4_BEEPS);
        }
      return 0;
      }
    }
  
  from = key_rec->FmDate & 0x3FFF;
  to   = key_rec->ToDate & 0x3FFF;
  if (from && from != 0xFFFF &&  from <= to)
    {
    today = RTC_ComputeDay(&sys_date_time);             // check if from/to date is OK
    if (today < from || today > to)
      {
      generate_event(source, key_rec->ID, key_rec->Code, EVT_Invalid_from_to_date);
      if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
        {
        set_buzzer(source, BZMODE_4_BEEPS);
        }
      return 0;
      }
    }
  return 1;
  }


//--------------------------------------------------------------------------
void update_all_doors(u16 count)
  {
  u16 ix;
  for (ix = 0; ix < MAX_DOORS; ix++)
  doors[ix].Local_param[LPRM_TRAFFIC_COUNT] = count;
  update_locals_in_eeprom = 10;
  }

//--------------------------------------------------------------------------
void execute_unlock(u16 source, u16 Flags)
  {
  send_locker_unlock_command(source);
  }

//--------------------------------------------------------------------------
bool check_disable_conditions(u16 source, u32 key, u32 ID, u16 Flags)
  {
  if (!(Flags & KFLAG_Master))
    {
    if (doors[source].DisableDoor == 0xDA)
      {
      generate_event(source, ID, key, EVT_Unlock_not_permitted);
      if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
        {
        set_buzzer(source, BZMODE_4_BEEPS);
        }
      return 1;
      }
    }
  if (doors[source].DisableTZ)
    {
    if (check_time_zone(doors[source].DisableTZ))
      {
      if (!(Flags & KFLAG_Master))
        {
        return 1;
        }
      }
    }
  return 0;
  }

//--------------------------------------------------------------------------
void start_pin_code_wait(u16 source, u32 index, KEY_RECORD *key_rec)
  {
  memcpy(&pin_key_rec[source], key_rec, sizeof(KEY_RECORD));
  collected_pin[source] = 0;
  collected_digits[source] = 0;
  wait_wiegand_digit[source] = 1;
  pin_index[source] = index;
  }

//--------------------------------------------------------------------------
void init_pin_only_wait(u16 source)
  {
  KEY_RECORD key_rec;
  start_pin_code_wait(source, -1, &key_rec);
  }

//--------------------------------------------------------------------------
bool pin_only_mode(u16 source)
  {
  bool ret = 0;
  if (doors[source].Flags & LFLAG_PIN_CODE)
    {
    ret = 1;
    }
  return ret;
  }

//--------------------------------------------------------------------------
void clear_pin_wait(u16 source)
  {
  collected_digits[source] = 0;
  wait_wiegand_digit[source] = 0;
  pin_index[source] = 0;
  if (pin_only_mode(source))
    {
    init_pin_only_wait(source);
    }
  }

//--------------------------------------------------------------------------
void handle_elevator(KEY_RECORD *key_rec)
  {
  u32 aln, alx;
  bool allowed = false;
  AL_RECORD al;
  if (key_rec->Flags & KFLAG_Master) // if master, allow all floors
    {
    generate_event(MyAddress, key_rec->ID, key_rec->Code, EVT_Valid_Entry);
    energize_all_elevator_relays();
    return;
    }
  for (alx = 0; alx < 2; alx++)
    {
    aln = key_rec->al[alx];
    if (aln < MAX_AL) // access level active?
      {
      DB_load_access_level(aln, &al);
      for (u32 idx = 0; idx < 2; idx++)
        {
        if (check_time_zone(al.weekzone[idx])) //from/to time check
          { 
          generate_event(MyAddress, key_rec->ID, key_rec->Code, EVT_Valid_Entry);
          energize_elevator_relays(aln);
          allowed = true;
          }
        }
      }
    }
  if (!allowed)
    {
    generate_event(MyAddress, key_rec->ID, key_rec->Code, EVT_Invalid_time);
    }
  }


//--------------------------------------------------------------------------
bool DposControlAllowsUnlock(u32 source)
  {
  if (!enable_reader[source])
    {
    if (!DB_search_key(key_code[source], &key_record[source]))
      {
      if (rdr4_active)
        {
        rdr4_red(source);
        }
      generate_event(source, 0, key_code[source], EVT_Invalid_key);
      if (rdr4_active)
        {
        delay_ms(50);
        rdr4_yellow(source);
        }
      }
    else
      {
      generate_event(source, key_record[source].ID, key_code[source], EVT_Unlock_not_permitted);
      }
    if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
      {
      set_buzzer(source, BZMODE_4_BEEPS);
      }
    return false;
    }
  return true;
  }

//--------------------------------------------------------------------------
void process_key(u32 source, u32 key, u16 special)
  {
  u32 index, flag;
  u8 APB_counter, interlock, count = 0;
//  bool personal;
  u16 current_count, threshold;
  bool second_reader = Reader2IsSource(source);
  bool dual_reader = test_door_flag(source, LFLAG_2RDR_SINGLE_DOOR);
  key_code[source] = key;
  
  latest_key_data = key_code[source];
  
 
  key_checked++;
  
  output_high(LED_RED);
  output_high(LED_GREEN);
  
//  if (special == 1)
//    {
//    goto tag_approved;
//    }
  
  if (prev_key[source] == key_code[source])
    {
    return;
    }
  
  if (test_door_flag(source, LFLAG_DPOS_RDREN))
    {
    if (!DposControlAllowsUnlock(source))
      {
      return;
      }
    }
  
  last_key_to[source] = 6; // in 100 mS increments
  UserKeyReady[source] = 0;
  
//  if (test_door_flag(source, LFLAG_DELAYED))
//    {
//    if (delayed_open_state[source] == 1)  // delay?
//      {
//      return;
//      }
//    }
//  
//  if (dual_reader && // is this a dual reader, two man rule door?
//      test_door_flag(source, LFLAG_TWO_MAN))            // if true and ...
//    {
//    if (second_reader)                                  // key detected on 2nd reader and
//      {
//      if (two_man_count[source] &&                      // this is the second key and
//          two_man_timer[source])                        // two man rule timer not zero yet
//        {
//        ClearReader2(source);
//        second_reader = false;
//        if (key_code[source] == first_key[source].Code)                // is key from 2nd reader equal PIN code?
//          {
//          generate_event(source, key_record[source].ID, key_code[source], EVT_Valid_Entry);
//          goto exec_unlock;
//          }
//        else
//          {
//          goto invalid_key;
//          }
//        }
//      else
//        {
//        return; // first key not presented or timed out
//        }
//      } // if NOT 2nd reader, fall through for processing of first key
//    else if (two_man_count[source])
//      {
//      two_man_timer[source] = 0;
//      two_man_count[source] = 0;
//      return;
//      }
//    }
//  
  prev_key[source] = key_code[source];
  
  // search key in data base
  index = DB_search_key(key_code[source], &key_record[source]);
  
  if (index) // tag found in DB
    {
//    if (check_disable_conditions(source, key_code[source], key_record[source].ID, key_record[source].Flags))
//      {
//      return;
//      }
    count = 0;
    
//    if (!allowed_to_open(source, &key_record[source]))
//      {
//      goto yellow;
//      }
    
//    personal = 0;
//    if (key_record[source].Flags & KFLAG_PersonalAL)
//      {
//      personal = 1;
//      }
    
    valid_door_found = 0;
    
//    if (elevator_system)
//      {
//      handle_elevator(&key_record[source]);
//      return;
//      }
    
    if (check_access_level((u16)source, (u16)key_record[source].al[0]))
      {
      
      
      // tag found
      // check if PIN code needed
      
          generate_event(source, key_record[source].ID, key_code[source], EVT_Valid_Entry);
exec_unlock:
      two_man_timer[source] = 0;
      two_man_count[source] = 0;
      if (unlock10[source] == 0 && auto_unlocked[source] == 0)  //NN 120822 V10
        {
        execute_unlock(source, key_record[source].Flags);
        }

      if (test_door_flag(source, LFLAG_UNLOCK_BUZ))
        {
        set_buzzer(source, BZMODE_ON);
        }
      }
    else // door open rejected
      {
      two_man_timer[source] = 0;
      two_man_count[source] = 0;
      if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
        {
        set_buzzer(source, BZMODE_4_BEEPS);
        }
      if (valid_door_found)
        {
        generate_event(source, key_record[source].ID, key_code[source], EVT_Invalid_time);
        if (rdr4_active)
          {
          rdr4_red(source);
          delay_ms(50);
          rdr4_yellow(source);
          }
        }
      else
        {
        handle_invalid_door(source);
        }
      yellow:
      set_led(source, LMODE_BLINK_4);
      }
    }
  else // tag NOT found in DB
    {
    invalid_key:
    if (rdr4_active)
      {
      rdr4_red(source);
      }
    generate_event(source, 0, key_code[source], EVT_Invalid_key);
    if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
      {
      set_buzzer(source, BZMODE_4_BEEPS);
      }
    if (rdr4_active)
      {
      delay_ms(50);
      rdr4_yellow(source);
      }
    }
  }

//--------------------------------------------------------------------------
void movement_detected(u32 source)
  {
  if (people_inside[source] == 0 && movement_timer[source] < -100) // nobody is supposed to be in the room. ALARM
    {
    generate_event(source, 0, 0, EVT_burglary);
    }
  }

//--------------------------------------------------------------------------
void dead_man_switch_handling(void)
  {
  u32 room;
  for (room = 0; room < MAX_DOORS; room++)
    {
    if (test_door_flag(room, LFLAG_DEAD_MAN_SWITCH))
      {
      if (movement_timer[room] > 0)
        {
        if (--movement_timer[room] == 0)
          {
          generate_event(room, 0, 0, EVT_dead_man);
          }
        }
      else if (movement_timer[room] < 0)
        { // check for movement when nobody is in the room
        movement_timer[room]--;
        }
      }
    }
  }

//--------------------------------------------------------------------------
void check_first_person_timers(void)
  {
  u32 room;
  for (room = 0; room < MAX_DOORS; room++)
    {
    if (first_person_timer[room]) // timer is active?
      {
      if (--first_person_timer[room] == 0) // timer timed out?
        {
        if (people_inside[room] == 1)
          {
          generate_event(room, 0, 0, EVT_single_person_inside);
          }
        }
      }
    if (first_in_the_morning_timer[room])
      {
      if (--first_in_the_morning_timer[room] == 0)
        {
        if (people_inside[room] == 1)
          {
          generate_event(room, 0, 0, EVT_single_person_inside);
          }
        }
      }
    }
  }

//--------------------------------------------------------------------------
u32 process_key_idx = 0, scan_keys = 0, scan_delayed_idx = 0, scan_delayed_10 = 0;

void handle_keys(void)
  {
  u16 seconds;
  u16 ix;
  if (UserKeyReady[process_key_idx])
    {
    UserKeyReady[process_key_idx] = 0;
    process_key(process_key_idx, new_key[process_key_idx], 0);
    }
  if (++process_key_idx >= MAX_DOORS)
    {
    process_key_idx = 0;
    }
  
  if ((Timer_100mS_Flags & Tmr_100mS_KEYS))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_KEYS);
    
    check_first_person_timers();
    dead_man_switch_handling();
    
    scan_keys = 63;
    if (++scan_delayed_10 >= 100) // scan timers once every 10 seconds
      {
      scan_delayed_10 = 0;
      scan_delayed_idx = MAX_DOORS;
      }
    for (ix = 0; ix < 2; ix++)
      {
      if (two_man_timer[ix])
        {
        if (--two_man_timer[ix] == 0)
          {
          two_man_count[ix] = 0; // cancel first key in two man rule mode
          two_man_first[ix] = 0;
          }
        }
      }
    
    if (sys_date_time.hour == 0 && sys_date_time.minute == 0)
      {
      if (reset_first_in_the_morning)
        {
        reset_first_in_the_morning = false;
        memset((u8 *)first_in_the_morning_occur, 0, sizeof(first_in_the_morning_occur));
        memset((u8 *)first_in_the_morning_timer, 0, sizeof(first_in_the_morning_timer));
        }
      }
    else if (sys_date_time.minute != 0)
      {
      reset_first_in_the_morning = true;
      }
        
    }
  
  if (scan_keys && (Timer_1mS_Flags & Tmr_1mS_KEYS))
    {
    Timer_1mS_Flags &= ~(Tmr_1mS_KEYS);
    scan_keys--;
    if (last_key_to[scan_keys])
      {
      if (!--last_key_to[scan_keys])
        {
        prev_key[scan_keys] = 0xFFFFFFFFL;
        }
      }
    }
  
  if (scan_delayed_idx && (Timer_5mS_Flags & Tmr_5mS_DELAYED_OPEN))
    {
    scan_delayed_idx--;
    Timer_5mS_Flags &= ~(Tmr_5mS_DELAYED_OPEN);
    if (delayed_open_state[scan_delayed_idx])
      {
      if (!--delayed_open_timer[scan_delayed_idx])
        {
        if (++delayed_open_state[scan_delayed_idx] == 2) // entering open window?
          {
          seconds = doors[scan_delayed_idx].Local_param[LPRM_DELAYED_OPEN] / 10;
          if (seconds < 60)
            {
            seconds = 60;
            }
          delayed_open_timer[scan_delayed_idx] = seconds;
          set_led(scan_delayed_idx, LMODE_FAST);
          }
        else // window is closed
          {
          delayed_open_state[scan_delayed_idx] = 0;
          set_led(scan_delayed_idx, LMODE_OFF);
          }
        }
      }
    }
  }

