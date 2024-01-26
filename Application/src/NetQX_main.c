#include "main.h"

#include "setup.h"
#include "string.h"
#include "stdio.h"

#include "STM32_EEPROM_SPI.h"

//#define DEBUG
//#define REQUEST_KEY_RECORD
//#define TEST_MATRIX

void InitWDG(void);
void send_pc_wakeup(void);
void EEPROM_SPI_INIT(SPI_HandleTypeDef * hspi);
void add_keys_to_DB(void);
void init_rdr4(void);
void handle_rdr4(void);
  


extern u32   dip_switches, seconds_from_reset;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern IWDG_HandleTypeDef hiwdg;
extern SPI_HandleTypeDef hspi1;
extern u32 first_in_the_morning_timer[MAX_DOORS];


u16  event_index, event_counter;
u16  event_report_index, event_report_count;
u16  event_not_sent, event_next_send;

u16  clear_apb_table;
u8 fire_condition[MAX_DOORS];
u8 Month_special_days[31]; // buffer for a full month of up to 31 days
u8 enable_reader[MAX_DOORS];

u8 last_key_to[MAX_DOORS];
u32 prev_key[MAX_DOORS];
u32 new_key[MAX_DOORS];
u8 UserKeyReady[MAX_DOORS];

u8 APB_table[MAX_KEY_RECORDS / 4];

__no_init u32 keys_in_eeprom;

u16 waiting_events;

u16 reload_door_setup;

u16 Ethernet_active;

u8 update_locals_in_eeprom;

u8 event_log[256];
u8 elx;


u8 update_internal_setup[MAX_DOORS];

u8 mul_t_lock = 0;

u16 auto_unlock_act, auto_unlock_ctr, auto_unlock_idx;
u32 auto_unlock_mask = 1;

SETUP_RECORD global_setup;

DOOR_RECORD doors[MAX_DOORS];
u16 auto_unlocked[MAX_DOORS];


u16 two_man_count[MAX_DOORS];
u16  two_man_timer[MAX_DOORS];
u32 two_man_first[MAX_DOORS];
u16  enable_first_user[MAX_DOORS];

u16  delayed_open_timer[MAX_DOORS];
u16  delayed_open_state[MAX_DOORS];

u16 interlock_disable_unlock[MAX_DOORS];

u16  ajar_timeout[MAX_DOORS];
u16  door_ajar[MAX_DOORS];
u16  check_ajar[MAX_DOORS];
u16  unlock10[ELEVATOR_DOORS];

u16  DPOS_levels[MAX_DOORS];
u16  DPOS_status[MAX_DOORS];
u16  debounce_dpos[MAX_DOORS];

u16  RTE_status[MAX_DOORS];
u16  RTE_pressed[MAX_DOORS];
u16  debounce_rte[MAX_DOORS];

u16  door_closed[MAX_DOORS];
u8 door_change_1[MAX_DOORS] = { 1,1,1,1 };

u16 aux_pattern[MAX_DOORS], aux_mask[MAX_DOORS], aux_timer[MAX_DOORS];

u16 scan_locks = MAX_DOORS;

u16 MyAddress;

u32 update_dt = 11;

u32 main_loop_counter;

u16 allow_write;                    

u16 battery_ok;
u16 check_battery;
u16 power_ok = 10;

__no_init u32  count_all_key;
u32 disable_wdg = 0;
u16 global_variable[GLOBAL_VARIABLES];

u32 fire_detected, send_fire_cancel, fire_debounce;

u16 tamper_detected = 1000, tamper_debounce;

u16 safe_door_mode;

u32 seconds_from_reset;

u32 elevator_system;

extern u32 key_delay_timer[];


//============ pin code processing =========================
KEY_RECORD pin_key_rec[MAX_DOORS];
u32 collected_pin[MAX_DOORS];
u16  collected_digits[MAX_DOORS];
u16  wait_wiegand_digit[MAX_DOORS];
u32 pin_index[MAX_DOORS];

//============ serial FLASH addresses ======================
//#define FLASH_BLOCK_SIZE       512L
/*#define KEYS_PER_BLOCK         (FLASH_BLOCK_SIZE / sizeof(KEY_RECORD))
#define FLASH_KEY_BLOCKS       ((MAX_KEY_RECORDS / KEYS_PER_BLOCK) + 1L)

#define ADDR_KEYS             4096L
#define ADDR_EVENTS           ((ADDR_KEYS + FLASH_KEY_BLOCKS * FLASH_BLOCK_SIZE + 512) & 0xFFFFFE00L)

#define PAGE_EVENTS           (ADDR_EVENTS / FLASH_BLOCK_SIZE)
#define EVENT_PAGES           (MAX_EVENTS / 32)
  */

  
  
u16 COM1_rcnt;
u16 COM1_rxi;
u16 COM1_rxo;
u16 COM1_rbuf[COM1_RX_LEN];
                                                 u8 comm_tbuf[COM1_TX_LEN];
u8 comm_buf[COM1_TX_LEN];
u8 comm_echo[COM1_TX_LEN];

u16 comm_timer;
u16 packet_idx;
u16 comm_state;

u16 ETH_detected = 5;

//====== date/time =========================================
DATE_TIME sys_date_time;
u8 day_of_week;

//u8 const max_day[13] = {  0 ,31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const u16 CummulativeMonthDays[12] = {  0, 31, 59, 90,120,151,
                                        181,212,243,273,304,334};
//=========== timer =======================================-
u32 Timer_1mS_Flags, Timer_1ms_counter;
u32 Timer_5mS_Flags;
u32 Timer_10mS_Flags, Timer_10mS_Cnt;
u32 Timer_100mS_Flags, Timer_100mS_Flags2, Timer_1sec_Flags;
u32 Timer_1mS_Count[MAX_DOORS],  Timer_10ms_count[MAX_DOORS]; // used for timeout purposes by handler functions
u32 Timer_100mS_Counter;
__no_init u8 unique_id[12] @ 0x1FFFF7B4;


u32 set_1sec_flags;


u8 KeySource[MAX_DOORS];

u8  rdr_interval[MAX_DOORS];
u8  ColumnParity[MAX_DOORS];
u32 BitCnt[MAX_DOORS];
u8  ReaderFreq[MAX_DOORS];

u8 reader_bitmask[MAX_DOORS];
u8 reader_byteidx[MAX_DOORS];

u8 reader_data[MAX_DOORS][MAX_READER_DATA_LEN];
u8 received_key[MAX_DOORS][5];

u8 *reader_byteptr[MAX_DOORS];

u8 WGND_kepress[MAX_DOORS];

u16 rdr4_active = 0;

//----------- MCR data--------------------------------------
u8 MCR_T1chr[MAX_DOORS];                  // track I character accumulator
bool CardReady[MAX_DOORS];
bool TrackOneDetected[MAX_DOORS];
bool raw_start[MAX_DOORS];



//======== LED and lock variables ==============================
u16  led_period[MAX_DOORS], led_pattern[MAX_DOORS], led_mask[MAX_DOORS];
u16  led_mode[MAX_DOORS], led_timer[MAX_DOORS];
u16  lock_unlocked[MAX_DOORS];
u16  alarm_trouble[MAX_DOORS];

u16  buzzer_mode[MAX_DOORS];
u16  buzzer_pattern[MAX_DOORS];
u16  buzzer_timer[MAX_DOORS];
u16  buzzer_mask[MAX_DOORS];


//======== externals =================================
extern u16  relay_pulse[MAX_DOORS];

//=========== communications ==============================-
#define PC_COM_BUF_SIZE 64


//======== prototypes =============================================
void clear_reader_outputs(void);


//============ source modules ====================================-

KEY_RECORD post_key;
u32 recv_keys = 0, last_function = 0;


//=============================================================================


//=============================================================================

u8 test_door_flag(u8 door, u32 flags)
  {
  u8 flag;
  flag = 0;
  if (doors[door].Flags & flags)
    {
    flag++;
    }
  return flag;
  }

//-----------------------------------------------------------------------------
void init_readers(void)
  {
  u16 idx, rdr4 = 0;
  get_analog_inputs(); // CHECK IF IN4 IS SHORTED TO GROUND
  if (analog_values[ANALOG_RDR4] < 500) // is this an RDR-4 system?
    {
    init_rdr4();
    rdr4 = 1;
    rdr4_active = 1;
    }

  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    if (rdr4)
      {
      doors[idx].Reader_type = RDR_RDR4;
      }
    else
      {
      switch (doors[idx].Reader_type)
        {
        case RDR_MAGCARD:
          MCR_init_magcard(idx);
          break;
        case RDR_WIEGAND:
          WGND_init(idx);
          break;
        default:
          doors[idx].Reader_type = RDR_WIEGAND;
          WGND_init(idx);
          break;
        }
      }
    led_timer[idx] = 0;
    led_pattern[idx] = 0;
    led_period[idx] = 0;
    UserKeyReady[idx] = 0;
    KeySource[idx] = 0;
    }


  
//  enable_ext_interrupt(15);
//  enable_ext_interrupt(10);
//  enable_ext_interrupt(0);
//  enable_ext_interrupt(8);
//  
//  enable_ext_interrupt(14);
//  enable_ext_interrupt(13);
//  enable_ext_interrupt(7);
//  enable_ext_interrupt(9);
  }                        
                           
//=============================================================================
void read_door_setup(u32 door)
  {
  DOOR_RECORD dr;
//  read_ext_eeprom(EE_LDR1, ADDR_DOORS + door * sizeof(DOOR_RECORD), (u8 *)&dr, sizeof(DOOR_RECORD));
  read_ext_eeprom(EE_LDR1, ADDR_DOORS + door * sizeof(DOOR_RECORD), (u8 *)&dr, sizeof(DOOR_RECORD));
  memcpy(&doors[door], &dr, sizeof(dr));
  ajar_timeout[door] = (u32)doors[door].AjarTime;
  }

//=============================================================================
void write_door_setup(u8 door)
  {
  u16 crc;
  DOOR_RECORD dr, dr2;
  if (allow_write == 2975)
    {
    crc = CalculateCRC((u8 *)&doors[door], sizeof(DOOR_RECORD)-2);
    doors[door].ErrorCheck = crc;
    memcpy(&dr, &doors[door], sizeof(DOOR_RECORD));
//    write_ext_eeprom(ADDR_DOORS + door * sizeof(DOOR_RECORD), (u8 *)&dr, sizeof(DOOR_RECORD));
rewrite:
    write_ext_eeprom(ADDR_DOORS + door * sizeof(DOOR_RECORD), (u8 *)&dr, sizeof(DOOR_RECORD));
    delay_ms(10);
    read_ext_eeprom(0, ADDR_DOORS + door * sizeof(DOOR_RECORD), (u8 *)&dr2, sizeof(DOOR_RECORD));
    if (memcmp((const void *)&dr, (const void *)&dr2, sizeof(DOOR_RECORD)))
      {
      goto rewrite;
      }
    ajar_timeout[door] = (u32)doors[door].AjarTime;
    }
  allow_write = 0;
  }

//=============================================================================
void update_locals(u8 door)
  {
  allow_write = 2975;
  write_door_setup(door);
  }

//=============================================================================
void store_mfg_defaults(void)
  {
  u16 idx, v;
  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    v = 50;
    doors[idx].Unlock_time = v;
    doors[idx].DoorGroup = 0;
    doors[idx].ReaderTZ = 0;
    doors[idx].AutoTZ = 0;
    doors[idx].DisableTZ = 0;
    doors[idx].Flags = 0;
    doors[idx].Reader_type = 3;
    doors[idx].AjarTime = 20;
    doors[idx].Alternate_mode = 0;
    doors[idx].First_digit = 1;
    doors[idx].Number_of_digits = 24;
    doors[idx].DisableDoor = 0;
    memset(&doors[idx].Local_param[0], 0, sizeof(doors[idx].Local_param));
    allow_write = 2975;
    update_internal_setup[idx] = 1;
    write_door_setup(idx);
    delay_ms(10);
    }
  }

//-----------------------------------------------------------------------------
void load_setup(void)
  {
  u16 ix;
  for (ix = 0; ix < MAX_DOORS; ix++)
    {
    read_door_setup(ix);
    UserKeyReady[ix] = 0;
    fire_condition[ix] = 0;
    update_internal_setup[ix] = 16;
    }

  if (doors[0].Flags == 0xFFFFFFFFL && doors[1].Flags == 0xFFFFFFFFL &&
      doors[0].Unlock_time == 0xFFFF && doors[1].Unlock_time == 0xFFFF &&
      doors[0].Reader_type == 0xFF && doors[1].Reader_type == 0xFF)
    {
    store_mfg_defaults();
    }
  }


//-----------------------------------------------------------------------------
u8 Wbuf[128], Rbuf[128];
void test_eeprom_code(void)
  {  
  static EepromOperations stat;
  
  output_low(SF_WP);
  
  EEPROM_SPI_INIT(&hspi1); 
  
  memset(Wbuf, 77, 64);
  
  stat = EEPROM_SPI_WriteBuffer(Wbuf, 0, 64);
  
  memset(Rbuf, 0, 64);
  
  stat = EEPROM_SPI_ReadBuffer(Rbuf, 0, 64);
  
  __NOP();
  
  
  }

//-----------------------------------------------------------------------------
void init_system(void)
  {
  init_ports();
  load_setup();
  if (test_door_flag(0, LFLAG_ELEVATOR_CONTROL))
    {
    elevator_system = 1;
    }
  }

//=============================================================================
void lock_handler(void)
  {
  if ((Timer_100mS_Flags & Tmr_100mS_SCAN))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_SCAN);
    scan_locks = MAX_DOORS;
    }

  if (scan_locks && (Timer_1mS_Flags & Tmr_1mS_SCAN))
    {
    Timer_1mS_Flags &= ~(Tmr_1mS_SCAN);
    scan_locks--;
    if (!auto_unlocked[scan_locks] && unlock10[scan_locks])             // if any of the solenoids is on,
      {
      if (unlock10[scan_locks] != 0xFFFF)
        {
        --unlock10[scan_locks];
        if (!unlock10[scan_locks])        // then shut it off after 10 seconds
          {                     // along with the LEDs
          relock(scan_locks);
          }
        }
      }
    if (relay_pulse[scan_locks])
      {
      if (!--relay_pulse[scan_locks])
        {
        operate_lock(scan_locks, 0);
        operate_aux(scan_locks, 0);
        }
      }
    }
  }

//=============================================================================
void execute_auto_unlock(u16 door)
  {
  if (doors[door].DisableDoor != 0xDA)
    {
    unlock_door(door);
    auto_unlocked[door] = 1;
    generate_event(door, 0, 0, EVT_Auto_door_open);
    }
  }

//=============================================================================
void check_auto_unlock(void)
  {
  u32 aln, door;
  bool valid;
  if ((Timer_1sec_Flags & Tmr_1sec_AUTOLOCK))
    {
    Timer_1sec_Flags  &= ~(Tmr_1sec_AUTOLOCK);

    if (++auto_unlock_ctr >= 10) // check every 10 seconds
      {
      auto_unlock_ctr = 0;
      auto_unlock_idx = 0;
      auto_unlock_act = 0xFF;
      auto_unlock_mask = 1;
      }
    }
  if (auto_unlock_idx < MAX_DOORS && (auto_unlock_act & auto_unlock_mask) && (Timer_100mS_Flags & Tmr_100mS_AUTOLOCK))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_AUTOLOCK);
    door = auto_unlock_idx;
    auto_unlock_act &= ~auto_unlock_mask;
    aln = doors[door].AutoTZ;
    if (!aln && auto_unlocked[door])
      {
      goto relock_door;
      }
    if (aln > 0 && aln <= MAX_AL)
      {

      if (doors[door].DisableDoor == 0xDA)
        {
        goto force_relock_door;
        }
      if ((valid = check_time_zone(doors[door].AutoTZ)) != 0 && !auto_unlocked[door])
        {
        if (test_door_flag(door, LFLAG_1ST_VALID)) // first valid tag opens auto unlock
          {
          enable_first_user[door] = 1;
          }
        else
          {
          execute_auto_unlock(door);
          }
        }
      else if (!valid)
        {
relock_door:
        enable_first_user[door] = 0;
        if (auto_unlocked[door])
          {
force_relock_door:
          relock(door);
          generate_event(door, 0, 0, EVT_Auto_door_close);
          auto_unlocked[door] = 0;
          }
        }
      }
    auto_unlock_idx++;
    auto_unlock_mask <<= 1;
    }
  }

//=============================================================================
#define CLEAR_APB_BLOCK_SIZE 32
void APB_handler(void)      //NetMX changes needed
  {
  if (clear_apb_table && (Timer_100mS_Flags & Tmr_100mS_CLEAR_APB))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_CLEAR_APB);
    if (clear_apb_table + CLEAR_APB_BLOCK_SIZE < sizeof(APB_table))
      {
      memset(&APB_table[clear_apb_table-1], 0, CLEAR_APB_BLOCK_SIZE);
      clear_apb_table += CLEAR_APB_BLOCK_SIZE;
      }
    else
      {
      if (clear_apb_table <= sizeof(APB_table))
        memset(&APB_table[clear_apb_table-1], 0, sizeof(APB_table) - clear_apb_table + 1);
      clear_apb_table = 0;
      }
    }
  }

//=============================================================================
//--------------------------------------------------------------------------
void wait_magcard(u32 rdr)
  {
  MCR_wait_magcard(rdr);
  }

//--------------------------------------------------------------------------
void wait_for_key(void)
  {
  u16 idx;

  if (Timer_1mS_Flags & Tmr_1mS_KEY_DELAY)
    {
    Timer_1mS_Flags &= ~Tmr_1mS_KEY_DELAY;
    for (idx = 0; idx < MAX_DOORS; idx++)
      {
      if (key_delay_timer[idx])
        {
        key_delay_timer[idx]--;
        }
      }
    }
  
  if (rdr4_active)
    {
    handle_rdr4();
    }
  else
    {
    for (idx = 0; idx < MAX_DOORS; idx++)
      {
      if (doors[idx].Reader_type == RDR_MAGCARD)
        {
        wait_magcard(idx);
        }
      else
        {
        WGND_wait(idx);
        }
      }
    }
  }

//-----------------------------------------------------------------------------

void background_tasks(void)
  {
  update_date_time();
  GKP_comm_handler();
  APB_handler();
  lock_handler();
  handle_rte();
  handle_doors();
  check_auto_unlock();
  handle_keys();
  handle_aux_relays();
  wait_for_key();
  led_handler();
  get_analog_inputs();
  battery_handler();
  buzzer_handler();
  }

//=============================================================================

void update_date_time(void)
  {
  u8 month;
  if ((Timer_100mS_Flags & Tmr_100mS_UPDT))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_UPDT);
    Timer_100mS_Counter++;
    if (++set_1sec_flags >= 10)
      {
      set_1sec_flags = 0;
      Timer_1sec_Flags = 0xFFFFFFFF;
      seconds_from_reset++;
      }
    if (++update_dt >= 100) // update date time from RTC every 10 seconds
      {
      update_dt = 0;
      month = sys_date_time.month;
      RTC_load_date_time();
      if (sys_date_time.month != month)
        {
        DB_load_holidays(sys_date_time.month, Month_special_days);
        }
      if (test_door_flag(0, LFLAG_APB_TOUT) && !clear_apb_table)
        {
        if (sys_date_time.minute > 55 && sys_date_time.hour == 23)
          {
          clear_apb_table = 1;
          }
        }
      if (sys_date_time.minute == 59 && sys_date_time.hour == 23 && 
          sys_date_time.second >= 40)
        {
        for (u32 idx = 0; idx < MAX_DOORS; idx++)
          {
          if (first_in_the_morning_timer[idx] == 1)
            {
            first_in_the_morning_timer[idx] = 0;
            }
          }
        }
      }
    }
  }

//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
void init_variables(void)
  {
  u16 idx;

  eeprom_write_timer = 0;

//  Dgroupx = 0;
//  number_of_doors = 0;
  clear_apb_table = 0;
  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    last_key_to[idx] = 0;
    prev_key[idx] = 0;
    new_key[idx] = 0;
    UserKeyReady[idx] = 0;
    enable_reader[idx] = 0;
    auto_unlocked[idx] = 0;
    two_man_count[idx] = 0;
    two_man_timer[idx] = 0;
    two_man_first[idx] = 0;
    enable_first_user[idx] = 0;
    delayed_open_timer[idx] = 0;
    delayed_open_state[idx] = 0;
    interlock_disable_unlock[idx] = 0;
    door_ajar[idx] = 0;
    check_ajar[idx] = 0;
    unlock10[idx] = 0;
    RTE_status[idx] = 0;
    DPOS_status[idx] = 0;
    DPOS_levels[idx] = 0;
    door_closed[idx] = 1;
    RTE_pressed[idx] = 0;
    ajar_timeout[idx] = 20;
    debounce_dpos[idx] = 0;
    buzzer_mode[idx]    = 0;
    buzzer_pattern[idx] = 0;
    buzzer_timer[idx]   = 0;
    buzzer_mask[idx]    = 0;
    if (pin_only_mode(idx))
      {
      init_pin_only_wait(idx);
      }
#ifndef DEBUG
//    read_door_setup(idx);
#else
    doors[idx].Unlock_time = 5;
    doors[idx].DoorGroup = 0;
    doors[idx].ReaderTZ = 0;
    doors[idx].AutoTZ = 0;
    doors[idx].DisableTZ = 0;
    doors[idx].Flags = LFLAG_PIN_CODE;
    doors[idx].Reader_type = 3;
    doors[idx].AjarTime = 20;
    doors[idx].Alternate_mode = 0;

#endif
    }
  }


//=============================================================================
void test_comm(void)
  {
  u32 dly = 0;
  delay_ms(2000);
  while (1)
    {
    output_low(LED_GREEN);
    output_low(LED_RED);
    ack_detected = 0;
    put_short(MSG_UNLK);
    put_short(2);
    COM1_send_packet(1, 0xF1, 7);
    while (!ack_detected)
      {
      background_tasks();
      delay_ms(1);
      if (++dly > 250)
        {
        output_toggle(LED_RED);
        dly = 0;
        }
      if (RTE_status[0] < 128 || RTE_status[1] < 128)
        {
        return;
        }
      }
    output_high(LED_RED);
    delay_ms(5000);
    output_low(LED_RED);
    delay_ms(2000);
    }
  }

//=============================================================================
void send_test_text(void)
  {
  u8 buf[40];
  sprintf((char *)buf, "NetDX V%lu\r\n", SW_VERSION);
  COM1_send_string(buf);
  }

//=============================================================================
void test_eeproms(void)
  {
  u8 wbuf[32], rbuf[32];
  u32 address = 0, index;
  for (index = 0; index < 16; index++)
    {
    wbuf[index] = 200 - index;
    }
  for (index = 0; index < 4; index++, address += 131072)
    {
    write_ext_eeprom(address, wbuf, 16);
    read_ext_eeprom(0, address, rbuf, 16);
    }
  }

//=============================================================================
bool test_reader_loopback(u32 reader)
  {
  led_onoff(reader, 1);
  delay_us(10);
  if (get_reader_d1(reader) == 0)
    {
    led_onoff(reader, 0);
    return false;
    }
  led_onoff(reader, 0);
  delay_us(10);
  if (get_reader_d1(reader) == 1)
    {
    return false;
    }
  return true;
  }

//=============================================================================
void clear_door1_flags(void)
  {
  doors[0].Flags = 0;
  allow_write = 2975;
  update_internal_setup[0] = 1;
  write_door_setup(0);
  }

//=============================================================================
void check_for_system_resets_on_powerup(void)
  {
  u32 rdr4 = rdr4_active;
  rdr4_active = 1; // disable while loop on inputs
  if (test_reader_loopback(3))  //loop on reader 4
    {
    ict_self_test();
    }
  else if (test_reader_loopback(0))//loop on reader 1
    {  // clear event buffer
    erase_events();
    }
  else if (test_reader_loopback(1))//loop on reader 2
    {  // clear user database and restore defaults
    store_mfg_defaults();
    erase_keys_in_DB();
    DB_erase_Al_wk_dy();
    }
  else if (test_reader_loopback(2))//loop on reader 3
    {
    clear_door1_flags();
    }
  rdr4_active = rdr4;
  }

void toggle_led1(void)
  {
  u32 period = 30, loop, time;
  for (loop = 0; loop < 1000; loop++)
    {
    
    for (time = 0; time < 300; time++) 
      {
      led_onoff(0,1);
      delay_us(period);
      led_onoff(0,0);
      delay_us(period);
      }
    period += 10;
    }
  }

//=============================================================================
void Init_APP_main(void)
  {
  static u32 max;
  u16 a1,a2,a3;
  
  init_IOs();
  
  output_high(LED_RED);
  output_high(LED_GREEN);
  
//  toggle_led1();

  delay_ms(1000);

  init_eeprom();

  delay_ms(400);

  clear_reader_outputs();
  
  init_variables();

  for (u32 door = 0; door < MAX_DOORS; door++)
    {
    set_aux(door, 0);
    door_buzzer(door, 0);
    }

//  reset_lantronix();

  COM1_init();

  RTC_init_chip();

  a1 = get_address();
  delay_us(50);
  a2 = get_address();
  delay_us(50);
  a3 = get_address();
  if (a1 == a2 || a1 == a3)
    {
    MyAddress = a1;
    }
  else if (a2 == a3)
    {
    MyAddress = a2;
    }

  init_system();

//  test_eeproms();
  
  check_for_system_resets_on_powerup();  

  if (count_all_key != 0x12345678)
    {
    count_keys_in_DB();
    }
  count_all_key = 0x12345678;

  init_event();

  RTC_load_date_time();
  if (sys_date_time.month && sys_date_time.month <= 12)
    {
    DB_load_holidays(sys_date_time.month, Month_special_days);
    }
  
  init_readers();

  for (u32 rdr = 0; rdr < MAX_DOORS; rdr++)
    {
    set_buzzer(0, BZMODE_OFF);
    clear_pin_wait(1);
    }
  
//  count_keys_in_DB();

  memset(&APB_table, 0, sizeof(APB_table));

  max = ADDR_LAST;
  __NOP();
  }

//=============================================================================
u32 max_addr = ADDR_LAST;
void APP_main(void)
  {
//  u32 last_ee;
//  KEY_RECORD urec;
//u16 pages, present;
//u8 buf[128];

  u16 toggle_led = 0, ix, fire_seconds_ctr = 0;
  u16 update_door_locals = 0xFF;
  u32 check_all_relays = 0;
  u32 maxa = max_addr;
  Init_APP_main();
  

  if (check_all_relays)
    {  
    test_relays();
    }
  
  TestMemory();

  initUart2();

  u8 chr = USART2->DR;
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

  
  while (1)
    {
    main_loop_counter++;
    
    background_tasks();
    if ((Timer_5mS_Flags & Tmr_5mS_TOGGLE_LED))
      {
      Timer_5mS_Flags &= ~(Tmr_5mS_TOGGLE_LED);

      check_fire_input();
      check_tamper();

      if (++toggle_led > 190)
        {
        MyAddress = get_address();

        if (!ETH_detected)
          {
          output_high(LED_RED);
          }
        else
          {
          output_high(LED_GREEN);
          }


        if (toggle_led >= 200)
          {
          if (disable_wdg != 0x1a2c3e5f)
            {
            InitWDG();
            }
          
          toggle_led = 0;
          if (ETH_detected) // check every second
            {
            if (--ETH_detected == 0)
              {
              send_pc_wakeup();
              }
            }
          if (update_locals_in_eeprom)
            {
            if (!--update_locals_in_eeprom)
              {
              update_door_locals = 0;
              }
            }
          if (update_door_locals < MAX_DOORS)
            {
            update_locals(update_door_locals++);
            }
          if (++reload_door_setup > 1000 && !update_locals_in_eeprom)
            {
            reload_door_setup = 0;
#ifndef DEBUG
            for (ix = 0; ix < MAX_DOORS; ix++)
              {
              read_door_setup(ix);
              }
#endif
            }

          if (++fire_seconds_ctr > 5) // send fire message every 5 seconds
            {
            fire_seconds_ctr = 0;
            if (fire_detected > 3)
              {
              fire_detected--;
              send_fire_alarm(1);
              }
            else if (send_fire_cancel)
              {
              send_fire_alarm(0);
              send_fire_cancel--;
              }
            }
          }
        }
      else
        {
        output_low(LED_GREEN);
        output_low(LED_RED);
        }
      }
    }
  }


void delay_us(u32 microsec)
  {
  while (microsec--)
    {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP();
    }
  }

void delay_ms(u32 millisec)
  {
  while (millisec--)
    {
    delay_us(1000);
    }
  }

void restart_wdt(void)
  {
  }

