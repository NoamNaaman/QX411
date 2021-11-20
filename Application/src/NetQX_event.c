#include "main.h"

#include "setup.h"
#include "stdlib.h"
#include "string.h"

void init_event(void);

//u32 save_event_place;

//--------------------------------------------------------------
u32 date_to_long(DATE_TIME *dt)
  {
  u32 l;
  l = 0;
  l = (u32)(dt->year - 8) << 26;
  l |= (u32)dt->month << 22;
  l |= (u32)dt->day << 17;
  l |= (u32)dt->hour << 12;
  l |= (u32)dt->minute << 6;
  l |= (u32)dt->second;
  return l;
  }

//--------------------------------------------------------------
void long_to_date(u32 compdt, DATE_TIME *dt)
  {
  dt->year = (make8(compdt, 3) >> 2) + 8;
  dt->month = (compdt >> 22) & 15;
  dt->day = (compdt >> 17) & 31;
  dt->hour = (compdt >> 12) & 31;
  dt->minute = (compdt >> 6) & 63;
  dt->second = compdt & 63;
  }

//--------------------------------------------------------------
u32 compute_event_address(u32 index)
  {
  return ADDR_EVENTS + (u32)index * EVENT_REC_SIZE;
  }

//--------------------------------------------------------------
void erase_events(void)
  {
  u16 index;
  u32 addr;
  u8 buf[EVENT_REC_SIZE];
  addr = compute_event_address(0);
  memset(buf, 0xFF, EVENT_REC_SIZE);
  for (index = 0; index < MAX_EVENTS; index++, addr += EVENT_REC_SIZE)
    {
    write_ext_eeprom(addr, buf, EVENT_REC_SIZE);
    }
  event_next_send = 0;
  write_ext_eeprom(ADDR_EVTPTR, (u8 *)&event_next_send, 2);
  event_not_sent = 0;
  }

//--------------------------------------------------------------

void generate_event(u8 source, u32 ID, u32 kcode, u8 event)
  {
  static EVENT_RECORD evt, evt2;
  u32 addr, event_idx;
  u8 FF = 0xFF;
  s32 not_sent, not_diff;
  
  //return;

  RTC_load_date_time();
//  memcpy(&evt.timestamp, &sys_date_time, sizeof(DATE_TIME));
  evt.event_type = event;
  evt.ID = ID;
  evt.source = source;
  evt.Code = kcode;
  evt.timestamp = date_to_long(&sys_date_time);

//  event_log[elx++] = event;
//  event_log[elx] = 0xFF;

  not_sent = abs((s32)event_index - (s32)event_next_send);
  not_diff = abs(event_not_sent - not_sent);
  if (event_index >= MAX_EVENTS)
    {
    event_index = 0;
    }
  else if (not_sent >= MAX_EVENTS || not_diff > 2)
    {
    event_next_send = (event_index - 10) % MAX_EVENTS;
    event_not_sent = 10;
    event_counter = 10;
    }

  addr = compute_event_address(event_index);
rewrite:
  write_ext_eeprom(addr, (u8 *)&evt, EVENT_REC_SIZE);
  delay_ms(5);
  read_ext_eeprom(EE_LEV, addr, (u8 *)&evt2, EVENT_REC_SIZE);
  if (memcmp((u8 *)&evt, (u8 *)&evt2, EVENT_REC_SIZE))
    {
    __NOP();
    goto rewrite;
    }
  
  
  event_idx = event_index;
  if (++event_index >= MAX_EVENTS)
    {
    event_index = 0;
    }
  
  addr = compute_event_address(event_index); // write an FF to mark end of event data
  write_ext_eeprom(addr, (u8 *)&FF, 1);

  if (event_counter < MAX_EVENTS)
    {
    event_counter++;
    }
  if (event_not_sent < MAX_EVENTS)
    {
    event_not_sent++;
    }
  else
    {
    event_next_send = event_index;
    }
  if (!ETH_detected)
    {
    waiting_events++;
    send_event(event_idx, &evt);
    process_incoming_event(comm_tbuf); // update other doors on this controller
    }
  }

//--------------------------------------------------------------
void get_event_record(u32 index, EVENT_RECORD *evt)
  {
  u32 addr;
  addr = compute_event_address(index);
  read_ext_eeprom(EE_LEV, addr, (u8 *)evt, EVENT_REC_SIZE);
  }

//--------------------------------------------------------------
void put_event_record(u32 index, EVENT_RECORD *evt)
  {
  u32 addr;
  addr = compute_event_address(index);
  write_ext_eeprom(addr, (u8 *)evt, EVENT_REC_SIZE);
  }

//--------------------------------------------------------------
void return_prev_event(EVENT_RECORD *evt)
  {
  if (event_report_count >= MAX_EVENTS || !event_counter)
    {
    evt->event_type = 0xFF;
    return;
    }
  get_event_record(event_report_index, evt);
  if (evt->event_type == 0xFF) // virgin memory?
    return;
  event_report_count++;
  if (event_report_index-- == 0)
    event_report_index = MAX_EVENTS - 1;
  }

//--------------------------------------------------------------
void init_event_report(void)
  {
  if (event_index == 0)
    event_report_index = MAX_EVENTS - 1;
  else
    event_report_index = event_index - 1;
  event_report_count = 0;
  }

//--------------------------------------------------------------
s32 compare_time(u8 *time1, u8 *time2)
  {
  u16 idx;
  for (idx = 0; idx < sizeof(DATE_TIME); idx++, time1++, time2++)
    if (*time1 > *time2)
      return 1;
    else if (*time1 < *time2)
      return -1;
  return 0;
  }

//--------------------------------------------------------------
u32 convert_dt_to_seconds(DATE_TIME *time1)
  {
  u32 t1;
  t1 = (time1->year * 365) + CummulativeMonthDays[time1->month] + time1->day;
  t1 = ((t1 * 24 + time1->hour) * 60 + time1->minute) * 60 + time1->second;
  return t1;
  }

//--------------------------------------------------------------
u32 date_time_diff(DATE_TIME *time1, DATE_TIME *time2)
  {
  u32 t1, t2;
  t1 = convert_dt_to_seconds(time1);
  t2 = convert_dt_to_seconds(time2);
  return t1 - t2;
  }

//--------------------------------------------------------------
void init_event(void)
  {
  EVENT_RECORD evt;
//  u32 prevtime;
//  DATE_TIME dt;
//  u32 l;
  u32 addr;
  u8 FF = 0xFF;

  output_low(LED_RED);
  output_high(LED_GREEN);
//  prevtime = 0;

  read_ext_eeprom(EE_EPT, ADDR_EVTPTR, (u8 *)&event_next_send , 2);
  if (event_next_send >= MAX_EVENTS) // never written?
    {
    event_next_send = 0;
    event_not_sent = 0;
    addr = compute_event_address(0); // write an FF to mark end of event data
    write_ext_eeprom(addr, (u8 *)&FF, 1);
    }

  restart_wdt();
  for (event_counter = 0, event_index = event_next_send; event_counter < MAX_EVENTS; event_counter++)
    {
    if ((event_counter & 15) == 0)
      {
      output_toggle(LED_RED);
      output_toggle(LED_GREEN);
      }
    addr = compute_event_address(event_index);
    read_ext_eeprom(EE_IEV, addr, (u8 *)&evt, EVENT_REC_SIZE);
    if (evt.event_type == 0xFF) // virgin memory?
      {
      break; // event table is not full yet.
      }
    if (++event_index >= MAX_EVENTS)
      event_index = 0;
    }
  event_not_sent = event_counter;
  }

//--------------------------------------------------------------
u16 check_event_index_validity(void)
  {
  if (event_next_send >= MAX_EVENTS) // out of bounds?
    {
    init_event();
    return 1;
    }
  return 0;
  }


