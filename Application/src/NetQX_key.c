#include "main.h"

#include "setup.h"

bool valid_door_found;
u32 latest_key_data, key_checked;

u32 key_delay_timer[MAX_DOORS];

u32 locker_access_level;


extern u32 elevator_system, WiegandReader2[];

void send_locker_unlock_command(u32 door);
void operate_relay(u32 relay, u32 onoff);
void send_locker_unlock_time(void);
bool Reader2IsSource(u32 door);
void ClearReader2(u32 door);
bool check_locker_validity(u32 locker, u32 aln);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
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
  u8 mask, floors;
  send_locker_unlock_time();
  DB_load_access_level(aln, &al);
  if (al.weekzone[0])
    {
    if (check_time_zone(al.weekzone[0] == 0))
      {
      return;
      }
    }
  
  for (index = 0, floors = al.ValidDoor[0][0], mask = 1; mask != 0; mask <<= 1, index++)
    {
    if ((floors & mask) != 0)
      {
      enable_floor(index);
      }
    }
  
  u32 floor_cnt = doors[0].Local_param[7];
  for (index = 1; index < 8; index++)
    {
    floors = al.ValidDoor[0][index];
    for (u32 floor = 1, mask = 1; floor < 9; mask <<= 1, floor++)
      {
      if ((floors & mask) != 0)
        {
        send_locker_unlock_command((index-1) * 8 + floor);
        }
      if (--floor_cnt == 0)
        {
        return;
        }
      }
    }
  }

//--------------------------------------------------------------------------
bool check_access_level(u32 door, u32 aln)
  {
  AL_RECORD al;
  u32 mask, albyteX, nibbleX;
  u32 idx;

#if __LOCKER_SYSTEM__ == 1      
  return true;
#endif
  
  if (!aln)
    {
    return 0;
    }
  
  albyteX = (MyAddress - 1) >> 3;
  nibbleX = (MyAddress - 1)  & 7;
  nibbleX = (nibbleX >= 4) ? 4 : 0;
  mask = 1 << (door + nibbleX);
  DB_load_access_level(aln, &al);
  for (idx = 0; idx < 2; idx++)
    {
    if (al.ValidDoor[idx][albyteX] & mask) // check if door is valid selection
      {
      valid_door_found = 1;
      if (check_time_zone(al.weekzone[idx])) //from/to time check
        {
        return 1;
        }
      }
    }
  return 0;
  }

//--------------------------------------------------------------------------
bool check_locker_validity(u32 locker, u32 aln)
  {
  AL_RECORD al;
  u32 mask, albyteX, nibbleX;
  u32 idx;

  if (!aln)
    {
    return 0;
    }
  
  locker--; // adjust locker # to zero base
  albyteX = locker >> 3;
  mask = 1 << (locker & 7);
  DB_load_access_level(aln, &al);
  for (idx = 0; idx < 2; idx++)
    {
    if (al.ValidDoor[idx][albyteX] & mask) // check if door is valid selection
      {
      valid_door_found = 1;
      if (check_time_zone(al.weekzone[idx])) //from/to time check
        {
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
  if (rdr4_active)
    {
    rdr4_green(source);
    }
  unlock_door(source);
  unlock10[source] = doors[source].Unlock_time;// * 10;
  if (Flags & KFLAG_ExtraUnlock)
    {
    unlock10[source] <<= 1; // make unlock time twice as long
    }
  if (enable_first_user[source] && (Flags & KFLAG_AutoOpenStart))
    {
    enable_first_user[source] = 0;
    auto_unlocked[source] = 1;
    generate_event(source, 0, 0, EVT_Auto_door_open);
    }
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
void process_key(u32 source, u32 key, u16 special)
  {
  static KEY_RECORD key_rec, first_key;
  u32 index, flag;
  u8 APB_counter, interlock, count = 0;
  bool personal;
  u16 mask, current_count, threshold, dual_reader, door = 0;
  bool second_reader = Reader2IsSource(source);
  u32 locker_number;
  
  locker_number = key;
  
  latest_key_data = key;
  
  if (key_delay_timer[source])
    {
    if (key_delay_timer[source] < 1000)
      {
      key_delay_timer[source] += 500;
      }
    return;
    }
  
  key_delay_timer[source] = 700;
  
  key_checked++;
  
  
  output_high(LED_RED);
  output_high(LED_GREEN);
  
  bool single_door = ((doors[source].Flags & LFLAG_2RDR_SINGLE_DOOR) != 0) || (analog_values[ANALOG_SINGLE_DOOR] < 500);

  if (special == 1)
    {
    goto tag_approved;
    }
  
  if (prev_key[source] == key)
    {
    return;
    }
  
  if (test_door_flag(source, LFLAG_DPOS_RDREN))
    {
    if (!enable_reader[source])
      {
      if (!DB_search_key(key, &key_rec))
        {
        if (rdr4_active)
          {
          rdr4_red(source);
          }
        generate_event(source, 0, key, EVT_Invalid_key);
        if (rdr4_active)
          {
          delay_ms(50);
          rdr4_yellow(source);
          }
        }
      else
        {
        generate_event(source, key_rec.ID, key, EVT_Unlock_not_permitted);
        }
      if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
        {
        set_buzzer(source, BZMODE_4_BEEPS);
        }
      return;
      }
    }
  
  last_key_to[source] = 6; // in 100 mS increments
  UserKeyReady[source] = 0;
  
  //  if (doors[source].Unlock_time && unlock10[source])  //NN 120822 V10
  //    return;
  
  if (test_door_flag(source, LFLAG_DELAYED))
    {
    if (delayed_open_state[source] == 1)  // delay?
      {
      return;
      }
    }
  
  if (test_door_flag(source, LFLAG_2RDR_SINGLE_DOOR) && // is this a dual reader, two man rule door?
      test_door_flag(source, LFLAG_TWO_MAN))            // if true and ...
    {
    if (second_reader)                                  // key detected on 2nd reader and
      {
      if (two_man_count[source] &&                      // this is the second key and
          two_man_timer[source])                        // two man rule timer not zero yet
        {
        ClearReader2(source);
        second_reader = false;
        if (key == first_key.Code)                // is key from 2nd reader equal PIN code?
          {
          generate_event(source, key_rec.ID, key, EVT_Valid_Entry);
          goto exec_unlock;
          }
        else
          {
          goto invalid_key;
          }
        }
      else
        {
        return; // first key not presented or timed out
        }
      } // if NOT 2nd reader, fall through for processing of first key
    else if (two_man_count[source])
      {
      two_man_timer[source] = 0;
      two_man_count[source] = 0;
      return;
      }
    }
  
  prev_key[source] = key;
  
  // search key in data base
  index = DB_search_key(key, &key_rec);
  
  if (index) // tag found in DB
    {
    if (check_disable_conditions(source, key, key_rec.ID, key_rec.Flags))
      {
      return;
      }
    count = 0;
    
    if (!allowed_to_open(source, &key_rec))
      {
      goto yellow;
      }
    personal = 0;
    if (key_rec.Flags & KFLAG_PersonalAL)
      {
      personal = 1;
      }
    valid_door_found = 0;
    
    if (elevator_system)
      {
      handle_elevator(&key_rec);
      return;
      }
    
    locker_access_level = key_rec.al[0];
    
    if (check_access_level((u16)source, (u16)key_rec.al[0]))
      {
      goto continue_checking;
      }
    if (!elevator_system && personal) ////// testing ONLY 2 DOORS?
      {
      if (source)
        {
        mask = 0x8000;
        }
      else
        {
        mask = 0x4000;
        }
      if (key_rec.FmDate & mask)
        {
        valid_door_found = 1;
        if (check_time_zone(key_rec.al[1]))
          {
          goto continue_checking;
          }
        }
      else
        {
        invalid_door:
        two_man_timer[source] = 0;
        two_man_count[source] = 0;
        generate_event(source, key_rec.ID, key, EVT_Invalid_door);
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
      goto yellow;
      }
    else if (key_rec.al[1] > 0 && key_rec.al[1] < MAX_AL && check_access_level((u16)source, (u16)key_rec.al[1]))
      {
      continue_checking:
      if (test_door_flag(source, LFLAG_TWO_MAN))
        {
        if (!two_man_count[source]) // is this first tag of two needed for opening?
          {
          two_man_first[source] = key_rec.ID & 0x00FFFFFFL;
          generate_event(source, key_rec.ID, key, EVT_First_key_valid);
          two_man_timer[source] = doors[source].Local_param[LPRM_TWO_MAN_WINDOW] * 10;
          first_key = key_rec;
          if (two_man_timer[source] == 0)
            {
            two_man_timer[source] = 100;
            }
          two_man_count[source] = 1;
          return;
          }
        else 
          {
          two_man_count[source] = 0;
          two_man_first[source] = 0;
          if ((test_door_flag(source, LFLAG_2RDR_SINGLE_DOOR) && key != (first_key.Code)) ||
              (!test_door_flag(source, LFLAG_2RDR_SINGLE_DOOR) && key == (first_key.Code))
             )
            {
            return;
            }
          key_rec = first_key;
          }
        }
      else if (test_door_flag(source, LFLAG_DELAYED))
        {
        if (!delayed_open_state[source])  // start delay?
          {
          delayed_open_timer[source] = doors[source].Local_param[LPRM_DELAYED_OPEN];
          delayed_open_state[source] = 1;
          set_led(source, LMODE_SLOW);
          
          return;
          }
        }
      
      two_man_count[source] = 0;
      
      if (test_door_flag(source, LFLAG_INTERLOCK))
        {
        interlock = interlock_disable_unlock[source];
        if (interlock)
          {
          generate_event(source, key_rec.ID, key, EVT_Unlock_not_permitted);
          if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
            {
            set_buzzer(source, BZMODE_4_BEEPS);
            }
          return;
          }
        }
      
      // tag found
      // check if PIN code needed
#if __LOCKER_SYSTEM__ != 1      
      if (!pin_only_mode(source) && test_door_flag(source, LFLAG_PIN_CODE))
#endif
        {
        start_pin_code_wait(source, index, &key_rec);
#if __LOCKER_SYSTEM__ == 1      
        init_locker_display();
        display_locker_query();
#endif
        return;
        }
      
tag_approved:
      ClearReader2(source);
      
#if __LOCKER_SYSTEM__ == 1
      if (check_locker_validity(locker_number, locker_access_level))
        {
        send_locker_unlock_command(locker_number - 1);
        generate_event(locker_number - 1, key_rec.ID, key, EVT_Valid_Entry);
        display_locker_open();
        }
      else
        {
        display_invalid_locker();
        generate_event(locker_number - 1, key_rec.ID, key, EVT_Invalid_door);
        }
      return;
#endif
      
      if (test_door_flag(source, LFLAG_EXIT) || second_reader)                           // is this door an exit door?
        { // this is an exit                                                            // YES.
        if (test_door_flag(source, (LFLAG_HARD_APB|LFLAG_SOFT_APB)))
          {
          APB_counter = get_apb_counter(index);
          if (test_door_flag(source, LFLAG_HARD_APB))                   // hard APB does no allow exit if no entry executed previously
            {
            if (APB_counter & (APB_EXIT_FLAG))
              {
              generate_event(source, key_rec.ID, key, EVT_APB_key_blocked);// YES. do not allow entry
              if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
                {
                set_buzzer(source, BZMODE_4_BEEPS);
                }
              two_man_timer[source] = 0;
              two_man_count[source] = 0;
              return;
              }
            }
          APB_counter = APB_EXIT_FLAG; // set counter;
          set_apb_counter(index, APB_counter);
          }
        
        if (global_variable[GVAR_UPDOWN_CTR])                         // down count entry/exit counter
          {
          global_variable[GVAR_UPDOWN_CTR]--;
          }
        
        generate_event(source, key_rec.ID, key, EVT_Valid_Exit);      // generate exit event
        
        if (test_door_flag(source,  LFLAG_TRAFFIC_LT))                  // does this door participate in traffic control?
          {
          if (doors[source].Local_param[LPRM_TRAFFIC_COUNT])                      // YES. decrement occupancy counter
            {
            doors[source].Local_param[LPRM_TRAFFIC_COUNT]--;
            update_locals_in_eeprom = 10;
            //            update_all_doors(doors[source].Local_param[LPRM_TRAFFIC_COUNT]);
            if (doors[source].Local_param[LPRM_TRAFFIC_COUNT] < doors[source].Local_param[LPRM_TRAFFIC_THRESH])
              {
              operate_aux(source, 0);                                             // turn off red light
              }
            }
          }
        }
      else // this is an entry
        {
        if (test_door_flag(source, LFLAG_HARD_APB|LFLAG_SOFT_APB))
          {
          APB_counter = get_apb_counter(index);
          if (APB_counter & (APB_ENTRY_FLAG))
            {
            generate_event(source, key_rec.ID, key, EVT_APB_key_blocked);// YES. do not allow entry
            if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
              {
              set_buzzer(source, BZMODE_4_BEEPS);
              }
            two_man_timer[source] = 0;
            two_man_count[source] = 0;
            return;
            }
          APB_counter = APB_ENTRY_FLAG; // set counter;
          set_apb_counter(index, APB_counter);
          }
        
        flag = doors[source].Flags & LFLAG_DONT_COUNT;
        if ((flag) == 0L)
          {
          if (key_rec.Flags & KFLAG_EntryLimit)
            {
            count = make8(key_rec.ID, 3); // get counter
            if (!count)
              {
              generate_event(source, key_rec.ID, key, EVT_Key_timed_out);
              if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
                {
                set_buzzer(source, BZMODE_4_BEEPS);
                }
              two_man_timer[source] = 0;
              two_man_count[source] = 0;
              return;
              }
            }
          }
        global_variable[GVAR_UPDOWN_CTR]++;                           // up count entry/exit counter
        
        if (test_door_flag(source,  LFLAG_TRAFFIC_LT))                  // does this door participate in traffic control?
          {
          current_count = doors[source].Local_param[LPRM_TRAFFIC_COUNT];
          threshold = doors[source].Local_param[LPRM_TRAFFIC_THRESH];
          if (current_count >= threshold)                   // YES. have we reach maximum allowed occupancy?
            {
            if (key_rec.Flags & KFLAG_Master)
              {
              goto open_gate;
              }
            operate_aux(source, 0xFFFF);                                            // YES. turn on red light
            generate_event(source, key_rec.ID, key, EVT_Traffic_limit);// YES. do not allow entry
            if (test_door_flag(source, LFLAG_SHORT_REJECT_BEEPS))
              {
              set_buzzer(source, BZMODE_4_BEEPS);
              }
            goto yellow;
            }
          else
            {
            open_gate:
            two_man_timer[source] = 0;
            two_man_count[source] = 0;
            current_count = ++doors[source].Local_param[LPRM_TRAFFIC_COUNT];
            update_locals_in_eeprom = 10;
            if (current_count >= threshold)                 // have we reach maximum allowed occupancy?
              {
              operate_aux(source, 0xFFFF);                                            // YES. turn on red light
              generate_event(source, key_rec.ID, key, EVT_Valid_Entry);
              goto exec_unlock;
              }
            }
          }
        
        if (!dual_reader)
          {
          two_man_timer[source] = 0;
          two_man_count[source] = 0;
          generate_event(source, key_rec.ID, key, EVT_Valid_Entry);
          }
        else
          {
          if (!door)
            {
            generate_event(source, key_rec.ID, key, EVT_Valid_Entry);
            }
          else
            {
            generate_event(source, key_rec.ID, key, EVT_Valid_Exit);
            }
          }
        
        if (count)
          {
          key_rec.ID -= 0x01000000L; // decrement counter
          DB_store_key(&key_rec, index);
          }
        }
      exec_unlock:
      two_man_timer[source] = 0;
      two_man_count[source] = 0;
      if (unlock10[source] == 0 && auto_unlocked[source] == 0)  //NN 120822 V10
        {
        execute_unlock(source, key_rec.Flags);
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
        generate_event(source, key_rec.ID, key, EVT_Invalid_time);
        if (rdr4_active)
          {
          rdr4_red(source);
          delay_ms(50);
          rdr4_yellow(source);
          }
        }
      else
        {
        goto invalid_door;
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
    generate_event(source, 0, key, EVT_Invalid_key);
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
    scan_keys = MAX_DOORS;
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

