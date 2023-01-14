#include "main.h"

#include "setup.h"

u16 analog_values[11];

u16  rte_idx, dpos_idx;
u16  delay_rte_on_powerup;
u16 charge_time;
u16  relay_pulse[MAX_DOORS];

u16 internal_buzzer_on = 0;
u16 internal_buzzer_period = 0;
u16 internal_buzzer_period_inc, internal_buzzer_period_max;

u32   dip_switches;


extern ADC_HandleTypeDef hadc1;

//#define JSH_DOOR_CLOSED 133
//

void led_onoff(u16 led, u8 onoff);
void set_led(u16 door, u8 cmd);
void init_ports(void);

//=============================================================================================
/**
* @brief  Sets a single I/O pin to any operational mode.
* @param  GPIOx:  Port A, B, C etc.
* @Param  Pin:    Pin on port 0..15
* @Param  Mode:   One of the modes in GPIOModeFunc_TypeDef
* @retval None
*/
void SetPinMode(GPIO_TypeDef* GPIOx, u16 Pin, GPIOModeFunc_TypeDef PortMode)
  {
  u32 mask1, mask3;
  u32 Mode, Out, Speed, Pull;
  
  mask1 = ~((u32)1 << Pin);
  mask3 = ~((u32)3 << (Pin * 2));
  
  Mode =  PortMode & 3;           Mode  <<= Pin * 2;
  Out =   ((u32)PortMode >> 4)  & 1;   Out   <<= Pin;
  Speed = ((u32)PortMode >> 8)  & 3;   Speed <<= Pin * 2;
  Pull =  ((u32)PortMode >> 12) & 3;   Pull  <<= Pin * 2;
  
  GPIOx->MODER   = (GPIOx->MODER   & mask3) | Mode;
  GPIOx->OTYPER  = (GPIOx->OTYPER  & mask1) | Out;
  GPIOx->OSPEEDR = (GPIOx->OSPEEDR & mask3) | Speed;
  GPIOx->PUPDR   = (GPIOx->PUPDR   & mask3) | Pull;
  }

//--------------------------------------------------------------------------
void door_buzzer(u16 door, u8 onoff)
  {
  if (onoff)
    {
    switch (door)
      {
      case 0: output_high(RDR_BUZZER1); break;
      case 1: output_high(RDR_BUZZER2); break;
      case 2: output_high(RDR_BUZZER3); break;
      case 3: output_high(RDR_BUZZER4); break;
      }
    }
  else
    {
    switch (door)
      {
      case 0: output_low(RDR_BUZZER1); break;
      case 1: output_low(RDR_BUZZER2); break;
      case 2: output_low(RDR_BUZZER3); break;
      case 3: output_low(RDR_BUZZER4); break;
      }
    }
  }
/*
output_low(XP1);
output_low(XP2);
*/

//--------------------------------------------------------------------------
u8 get_address(void)
  {
  //  bool mosi, wp, hold;
  u16 addr, ret;
  //  u16 idx;
  
  input_pullup(DIP3);
  input_pullup(DIP4);
  input_pullup(DIP5);
  input_pullup(DIP6);
  input_pullup(DIP7);
  
  addr = 0;
  if (input(DIP3))
    {
    addr |= 4;
    }
  if (input(DIP4))
    {
    addr |= 8;
    }
  if (input(DIP5))
    {
    addr |= 16;
    }
  if (input(DIP6))
    {
    addr |= 32;
    }
  if (input(DIP7))
    {
    addr |= 64;
    }
  
  ret = addr;
//  ret ^= 0xFF;
  dip_switches = ret;
  ret = (ret & 0x007C) + 1;
  
  init_ports();
  return ret;
  }

//--------------------------------------------------------------------------
void operate_aux(u16 door, u16 onoff)
  {
  if (onoff)
    {
    switch (door)
      {
      case 0: output_high(AUX_RELAY1); break;
      case 1: output_high(AUX_RELAY2); break;
      case 2: output_high(AUX_RELAY3); break;
      case 3: output_high(AUX_RELAY4); break;
      }
    }
  else
    {
    switch (door)
      {
      case 0: output_low(AUX_RELAY1); break;
      case 1: output_low(AUX_RELAY2); break;
      case 2: output_low(AUX_RELAY3); break;
      case 3: output_low(AUX_RELAY4); break;
      }
    }
  if (onoff > 1)
    {
    aux_timer[door] = onoff;
    }
  }

//--------------------------------------------------------------------------
void operate_lock(u16 door, u8 onoff)
  {
  if (onoff)
    {
    switch (door)
      {
      case 0: output_high(LOCK_RELAY1); break;
      case 1: output_high(LOCK_RELAY2); break;
      case 2: output_high(LOCK_RELAY3); break;
      case 3: output_high(LOCK_RELAY4); break;
      }
    }
  else
    {
    switch (door)
      {
      case 0: output_low(LOCK_RELAY1); break;
      case 1: output_low(LOCK_RELAY2); break;
      case 2: output_low(LOCK_RELAY3); break;
      case 3: output_low(LOCK_RELAY4); break;
      }
    }
  }

//--------------------------------------------------------------------------
void operate_relay(u32 relay, u32 onoff)
  {
  if (relay & 1) // is this an aux relay?
    {
    operate_aux(relay / 2, onoff);
    }
  else
    {
    operate_lock(relay / 2, onoff);
    }
  }

//--------------------------------------------------------------------------
void test_relays(void)
  {
  operate_lock(0, 1);
  operate_lock(1, 1);
  operate_lock(2, 1);
  operate_lock(3, 1);
  
  delay_us(30000);
  
  operate_lock(0, 0);
  operate_lock(1, 0);
  operate_lock(2, 0);
  operate_lock(3, 0);
  
  operate_aux(0, 1);
  operate_aux(1, 1);
  operate_aux(2, 1);
  operate_aux(3, 1);
  
  delay_us(30000);
  
  operate_aux(0, 0);
  operate_aux(1, 0);
  delay_us(30000);
  operate_aux(2, 0);
  operate_aux(3, 0);
  
//  output_high(RELAY9);
//  output_high(RELAY10);
  
  delay_us(30000);
  
//  output_low(RELAY9);
//  output_low(RELAY10);
  }

//--------------------------------------------------------------------------
void unlock_door(u32 door)
  {
  operate_lock(door, 1);
  set_led(door, LMODE_ON);
  //  if (rdr4_active)
  //    {
  //    rdr4_green(door);
  //    }
  unlock10[door] = (u16)doors[door].Unlock_time; // relock automatically after 10 seconds
  lock_unlocked[door] = 1;
  if (test_door_flag(door, LFLAG_BIDIR_MOT))
    {
    relay_pulse[door] = 60;
    }
  }

//--------------------------------------------------------------------------
void set_aux(u16 door, u16 pattern)
  {
  aux_timer[door] = pattern;
  aux_pattern[door] = 0;
  aux_mask[door] = 0;
  if (!pattern)
    {
    operate_aux(door, 0);
    }
  else
    {
    aux_pattern[door] = pattern;
    aux_mask[door] = 0x4000;
    aux_timer[door] = 0;
    }
  }

//-----------------------------------------------------------------------------
void handle_aux_relays(void)
  {
  u16 idx;
//  if ((Timer_1sec_Flags & Tmr_1sec_AUX_RELAYS))
//    {
//    Timer_1sec_Flags  &= ~(Tmr_1sec_AUX_RELAYS);
//    for (idx = 0; idx < MAX_DOORS; idx++)
//      {
      //      if (aux_pattern[idx])
      //        {
      //        if (aux_pattern[idx] & aux_mask[idx])
      //          {
      //          operate_aux(idx, 1);
      //          }
      //        else
      //          {
      //          operate_aux(idx, 0);
      //          }
      //        aux_mask[idx] >>= 1;
      //        if (!aux_mask[idx])
      //          {
      //          if (aux_pattern[idx] & 0x8000)
      //            {
      //            aux_mask[idx] = 0x4000;
      //            }
      //          else
      //            {
      //            aux_mask[idx] = 0;
      //            aux_pattern[idx] = 0;
      //            }
      //          }
      //        }
      //      else 
  if ((Timer_100mS_Flags & Tmr_100mS_AUX_RELAYS))
    {
    Timer_100mS_Flags  &= ~(Tmr_100mS_AUX_RELAYS);
    for (idx = 0; idx < MAX_DOORS; idx++)
      {
      
      if (aux_timer[idx])
        {
        if (aux_timer[idx] == 0xFFFF)
          {
          continue;
          }
        --aux_timer[idx];
        if (aux_timer[idx] == 0)
          {
          operate_aux(idx, 0);
          }
        }
      }
    }
  }

//-----------------------------------------------------------------------------
void led_onoff(u16 led, u8 onoff)
  {
  if (!onoff)
    {
    switch (led)
      {
      case 0: output_low(READER_LED1); break;
      case 1: output_low(READER_LED2); break;
      case 2: output_low(READER_LED3); break;
      case 3: output_low(READER_LED4); break;
      }
    }
  else
    {
    switch (led)
      {
      case 0: output_high(READER_LED1); break;
      case 1: output_high(READER_LED2); break;
      case 2: output_high(READER_LED3); break;
      case 3: output_high(READER_LED4); break;
      }
    }
  }

//-----------------------------------------------------------------------------
void set_led(u16 door, u8 cmd)
  {
  switch (cmd)
    {
    case LMODE_OFF:
    led_mode[door] = 0;
    led_pattern[door] = 0;
    led_timer[door] = 1;
    led_mask[door] = 0;
    break;
    case LMODE_ON:
    led_mode[door] = 0;
    led_pattern[door] = 0xFFFF;
    led_timer[door] = 0;
    led_mask[door] = 1;
    break;
    case LMODE_SLOW:
    led_mode[door] = 0;
    led_pattern[door] = 7;
    led_timer[door] = 0;
    led_mask[door] = 1;
    break;
    case LMODE_FAST:
    led_mode[door] = 0;
    led_pattern[door] = 0x0303;
    led_timer[door] = 0;
    led_mask[door] = 1;
    break;
    case LMODE_BLINK_4:
    led_mode[door] = 0;
    led_pattern[door] = 3;
    led_timer[door] = 4;
    led_mask[door] = 1;
    break;
    }
  }


//-----------------------------------------------------------------------------
void led_handler(void)
  {
  u16 idx;
  
  if (!(Timer_100mS_Flags & Tmr_100mS_RDR_LEDS))
    {
    return;
    }
  
  Timer_100mS_Flags  &= ~(Tmr_100mS_RDR_LEDS);
  
  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    if (led_pattern[idx] & led_mask[idx])
      {
      led_onoff(idx, 1);
      if (led_timer[idx])
        {
        if (!--led_timer[idx])
          {
          led_mask[idx] = 0;
          led_pattern[idx] = 0;
          led_mode[idx] = 0;
          }
        }
      }
    else
    led_onoff(idx, 0);
    
    led_mask[idx] <<= 1;
    if (!led_mask[idx])
      {
      led_mask[idx] = 1;
      }
    }
  }

//-----------------------------------------------------------------------------
void set_buzzer(u32 door, BZ_MODES cmd)
  {
  switch (cmd)
    {
    case BZMODE_OFF:
      buzzer_mode[door] = 0;
      buzzer_pattern[door] = 0;
      buzzer_timer[door] = 1;
      buzzer_mask[door] = 0;
      break;
    case BZMODE_ON:
      buzzer_mode[door] = 0;
      buzzer_pattern[door] = 0xFFFF;
      buzzer_timer[door] = 0;
      buzzer_mask[door] = 1;
      break;
    case BZMODE_TROUBLE:
      buzzer_mode[door] = 0;
      buzzer_pattern[door] = 7;
      buzzer_timer[door] = 0;
      buzzer_mask[door] = 1;
      break;
    case BZMODE_ALARM:
      buzzer_mode[door] = 0;
      buzzer_pattern[door] = 0x0C1F;
      buzzer_timer[door] = 0;
      buzzer_mask[door] = 1;
      break;
    case BZMODE_4_BEEPS:
      buzzer_mode[door] = 0;
      buzzer_pattern[door] = 0x0003;
      buzzer_timer[door] = 4;
      buzzer_mask[door] = 1;
      break;
    case BZMODE_1MINUTE:
      buzzer_pattern[door] = 0x0C1F;
      buzzer_timer[door] = 600;
      break;
    }
  }

//-----------------------------------------------------------------------------
void buzzer_handler(void)
  {
  u16 idx;
  
  //  if ((Timer_10mS_Flags & Tmr_10mS_INT_BUZZER))
  //    {
  //    Tmr_10mS_INT_BUZZER = 0;
  //    if (internal_buzzer_on)
  //      {
  //      setup_buzzer(5000 - internal_buzzer_period);
  //      internal_buzzer_period = (internal_buzzer_period + internal_buzzer_period_inc) % internal_buzzer_period_max;
  //      }
  //    }
  
  
  if (!(Timer_100mS_Flags & Tmr_100mS_RDR_BUZZERS))
    {
    return;
    }
  
  Timer_100mS_Flags  &= ~(Tmr_100mS_RDR_BUZZERS);
  
  for (idx = 0; idx < MAX_DOORS; idx++)
    {
    if (buzzer_pattern[idx] & buzzer_mask[idx])
      {
      door_buzzer(idx, 1);
      if (buzzer_timer[idx])
        {
        if (!--buzzer_timer[idx])
          {
          buzzer_mask[idx] = 0;
          buzzer_pattern[idx] = 0;
          buzzer_mode[idx] = 0;
          }
        }
      }
    else
      {
      door_buzzer(idx, 0);
      }
    
    buzzer_mask[idx] <<= 1;
    if (!buzzer_mask[idx])
      {
      buzzer_mask[idx] = 1;
      }
    }
  }

//--------------------------------------------------------------------------
void relock(u16 door)
  {
  operate_lock(door, 0);
  unlock10[door] = 0; // relock
  lock_unlocked[door] = 0;
  set_led(door, LMODE_OFF);
  if (rdr4_active)
    {
    rdr4_yellow(door);
    }
  if (test_door_flag(door, LFLAG_BIDIR_MOT))
    {
    operate_aux(door, 0xFFFF);
    relay_pulse[door] = 60;
    }
  if (doors[door].DoorGroup && (doors[door].Flags & LFLAG_INTERLOCK))
    {
    generate_event(door, 0, 0, EVT_relocked);
    }
  if (test_door_flag(door, LFLAG_UNLOCK_BUZ))
    {
    set_buzzer(door, BZMODE_OFF);
    }
  }


u32 get_rte(u32 door)
  {
  switch (door)
    {
    case 0: return input(D1RTE); 
    case 1: return input(D2RTE); 
    case 2: return input(D3RTE); 
    case 3: return input(D4RTE); 
    }
  return 0;
  }

u32 get_dpos(u32 door)
  {
  switch (door)
    {
    case 0: return input(D1DPOS);
    case 1: return input(D2DPOS);
    case 2: return input(D3DPOS);
    case 3: return input(D4DPOS);
    }
  return 0;
  }

//=============================================================================
void handle_rte(void)
  {
  static u16 RTE_level;
  u16 normally_closed;
  
  if (!(Timer_5mS_Flags & Tmr_5mS_RTE))
    {
    return;
    }
  if (delay_rte_on_powerup < 100)
    {
    delay_rte_on_powerup++;
    return;
    }
  
  Timer_5mS_Flags &= ~(Tmr_5mS_RTE);
  
  if (unlock10[rte_idx] || auto_unlocked[rte_idx])
    {
    goto next_door;
    }
  
  if (doors[rte_idx].ReaderTZ)
    {
    if (!check_time_zone(doors[rte_idx].ReaderTZ))
      {
      goto next_door;
      }
    }
  if (doors[rte_idx].DoorGroup && (doors[rte_idx].Flags & LFLAG_INTERLOCK))
    {
    if (interlock_disable_unlock[rte_idx])
      {
      goto next_door;
      }
    }
  
  RTE_level = !get_rte(rte_idx);
  normally_closed = test_door_flag(rte_idx, LFLAG_RTE_NC) != 0;
  
  if ((RTE_level && !normally_closed) || (!RTE_level && normally_closed)) // not pressed?
    {
    RTE_pressed[rte_idx] = 0;
    debounce_rte[rte_idx] = 0;
    }
  else
    {
    if (RTE_pressed[rte_idx])
      {
      goto next_door;
      }
    if (++debounce_rte[rte_idx] < 7)
      {
      goto next_door;
      }
    
    generate_event(rte_idx, 0, 0, EVT_Request_to_Exit);
    unlock10[rte_idx] = doors[rte_idx].Unlock_time;// * 10;
    if (unlock10[rte_idx] < 5)
      {
      unlock10[rte_idx] = 5;
      }
    unlock_door(rte_idx);
    RTE_pressed[rte_idx] = 1;
    }
  next_door:
  if (++rte_idx >= MAX_DOORS)
    {
    rte_idx = 0;
    }
  }

//=============================================================================
void handle_dpos(void)
  {
  //  u16 door_pos;
  u16 DPOS_level;
  //  bool normally_closed;
  
  DPOS_level = get_dpos(dpos_idx);
  
  if (DPOS_level == DPOS_levels[dpos_idx])     // debounce DPOS
    {
    debounce_dpos[dpos_idx] = 0;
    return;
    }
  
  if (++debounce_dpos[dpos_idx] < 100)
    {
    return;
    }
  DPOS_levels[dpos_idx] = DPOS_level;
  }

//=============================================================================
void handle_doors(void)
  {
  u16 door_pos;
  u16 DPOS_level;
  bool normally_closed;
  
  if (!(Timer_5mS_Flags & Tmr_5mS_DPOS))
    {
    return;
    }
  
  handle_dpos();
  
  if (auto_unlocked[dpos_idx] || unlock10[dpos_idx] == 65535)
    {
    goto next_door;
    }
  
  Timer_5mS_Flags &= ~(Tmr_5mS_DPOS);
  
  door_pos = door_closed[dpos_idx] != 0;
  DPOS_level = !DPOS_levels[dpos_idx];
  normally_closed = test_door_flag(dpos_idx, LFLAG_DPOS_NO) == 0;
  
  //==== door ajar testing ==============================================
  if (!door_closed[dpos_idx] && check_ajar[dpos_idx])
    {
    if ((Timer_1sec_Flags & Tmr_1sec_DOOR_AJAR))
      {
      Timer_1sec_Flags  &= ~(Tmr_1sec_DOOR_AJAR);
      if (!unlock10[dpos_idx] && door_ajar[dpos_idx] < ajar_timeout[dpos_idx])
        {
        if (++door_ajar[dpos_idx] == ajar_timeout[dpos_idx])
          {
          generate_event(dpos_idx, 0, 0, EVT_Door_left_open);

          if (test_door_flag(dpos_idx, LFLAG_DAJAR_REL2))
            {
            operate_aux(dpos_idx, 300); // turn aux relay on for 30 seconds
            }
          alarm_trouble[dpos_idx] = 1;
          set_buzzer(dpos_idx, BZMODE_TROUBLE);
          }
        }
      }
    }
  
  //========= door position check =========================================
  if ((DPOS_level && !normally_closed) || (!DPOS_level && normally_closed))
    {
    set_buzzer(dpos_idx, BZMODE_OFF);
    door_closed[dpos_idx] = 1;
    door_ajar[dpos_idx] = 0;
    check_ajar[dpos_idx] = 0;
    if (test_door_flag(dpos_idx, LFLAG_DPOS_RDREN))
      {
      enable_reader[dpos_idx] = 0;
      goto next_door;
      }
    }
  else
    {
    door_closed[dpos_idx] = 0;
    if (test_door_flag(dpos_idx, LFLAG_DPOS_RDREN))
      {
      enable_reader[dpos_idx] = 1;
      goto next_door;
      }
    }
  
  //========= door position change handling =================================
  if (door_closed[dpos_idx] != door_pos) // dpos input changed polarity?
    {                                    // YES
    /*    if (door_change_1[dpos_idx])          // 1st change since reset?
    {
    door_change_1[dpos_idx]--;        // yes. simply store state
    door_closed[dpos_idx] = door_pos;
    goto next_door;
    }*/
    if (!door_closed[dpos_idx])          // did door just open?
      {                                  // YES
      if (!unlock10[dpos_idx])           // is this a force door event?
        {                                // YES
        if (!test_door_flag(dpos_idx, LFLAG_IGNORE_FORCED_DOOR))
          {
          if (door_change_1[dpos_idx])          // 1st change since reset?
            {
            door_change_1[dpos_idx]--;        // yes. simply store state
            goto next_door;
            }
          generate_event(dpos_idx, 0, 0, EVT_Door_forced_open);  //YES. sound alarm
          alarm_trouble[dpos_idx] = 2;
          if (test_door_flag(dpos_idx, LFLAG_DAJAR_REL2))
            {
            operate_aux(dpos_idx, 300); // turn aux relay on for 30 seconds
            }
          set_buzzer(dpos_idx, BZMODE_ALARM);
          }
        }
      else
        {
        unlock10[dpos_idx] = 5;            // this is a legal door open event. relock shortly
        check_ajar[dpos_idx] = (u16)1;
        if (doors[dpos_idx].DoorGroup && test_door_flag(dpos_idx, LFLAG_INTERLOCK))
          {
          generate_event(dpos_idx, 0, 0, EVT_door_opened);
          }
        }
      }  // end door opened
    
    
    else
      {                                  // door just closed!
      if ((alarm_trouble[dpos_idx]) || doors[dpos_idx].DoorGroup && test_door_flag(dpos_idx, LFLAG_INTERLOCK))
      generate_event(dpos_idx, 0, 0, EVT_door_closed);
      if (alarm_trouble[dpos_idx] != 2)
        {
        set_buzzer(dpos_idx, BZMODE_OFF);
        }
      else
        {
        set_buzzer(dpos_idx, BZMODE_1MINUTE);
        }
      alarm_trouble[dpos_idx] = 0;
      check_ajar[dpos_idx] = 0;
      }
    }
  
  //========= switch to other door =============================
  next_door:
  if (++dpos_idx >= MAX_DOORS)
    {
    dpos_idx = 0;
    }
  }


//=============================================================================
void set_adc_channel(u32 channel)
  {
  ADC1->SQR1 = 0;
  ADC1->SQR2 = 0;
  ADC1->SQR3 = channel;
  }

//=============================================================================
u32 read_adc(void)
  {
  u32 value;
  //  HAL_StatusTypeDef status;
  
  /**start conversion */
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  
  /**get value */
  delay_us(10);
  value = HAL_ADC_GetValue(&hadc1);
  return value;
  }


//=============================================================================
void get_analog_inputs(void)
  {
  u16 ix;
  if ((Timer_100mS_Flags & Tmr_100mS_SAMPLE))
    {
    Timer_100mS_Flags &= ~(Tmr_100mS_SAMPLE);
    for (ix = 0; ix < 4; ix++)
      {
      set_adc_channel(10+ix);
      delay_us(20);
      analog_values[ix] = read_adc();
      }
    for (ix = 0; ix < 2; ix++)
      {
      set_adc_channel(14+ix);
      delay_us(20);
      analog_values[4+ix] = read_adc();
      }
    for (ix = 0; ix < 2; ix++)
      {
      set_adc_channel(8+ix);
      delay_us(20);
      analog_values[6+ix] = read_adc();
      }
    ix++;
    ix++;
    ix++;
    }
  }

//-----------------------------------------------------------------------------
/*
BATV[0] VALUES
14.00V = 193
13.50V = 186
13.00V = 178
12.49V = 172
12.23V = 169
12.20V = 169
11.96V = 166
11.70V = 161
11.25V = 154
9.50V = 131
9.00V = 124
8.50V = 117
8.25V = 113
8.00V = 111
7.68V = 106
7.45V = 103
7.00V =  97
6.50V =  89
6.00V =  83
*/



void battery_handler(void)
  {
  u16 mainv[2], batv[2];
//  u16 door;
  //  output_low(V_DROP); // high voltage operation
  if ((Timer_1sec_Flags & Tmr_1sec_CHG_BATTERY))
    {
    Timer_1sec_Flags  &= ~(Tmr_1sec_CHG_BATTERY);
    set_adc_channel(1);
//    door = 0;
    delay_us(20);
    batv[0] = read_adc();
    set_adc_channel(0);
    delay_us(20);
    mainv[0] = read_adc();
    if (mainv[0] > 1100) // main power is available
      {
      if (power_ok < 10)
        {
        if (++power_ok >= 10)
          {
          generate_event(0, 0, 0, EVT_power_Restored);
          }
        }
      }
    else
      {
      if (!battery_ok)
        {
        //          output_low(BAT_CHG); // don't charge battery
        //          output_high(V_DROP); // low main voltage operation
        delay_us(20);
        set_adc_channel(1);
        delay_us(20);
        batv[1] = read_adc();
        //          output_low(V_DROP); // high main voltage operation
        if (batv[1] >= 2500) // battery >= 12V?
          {
          battery_ok = 1;
          generate_event(0, 0, 0, EVT_battery_Restored);
          }
        }
      else
        {
        set_adc_channel(1);
        delay_us(20);
        batv[1] = read_adc();
        if (battery_ok && batv[1] < 2400)
          {
          battery_ok = 0;
          generate_event(0, 0, 0, EVT_battery_Fault);
          }
        }
      }
    }
//  else
//    {
//    if (power_ok)
//      {
//      if (!--power_ok)
//        {
//        generate_event(door, 0, 0, EVT_power_fault);
//        }
//      }
//    //      output_low(BAT_CHG); // don't charge battery
//    //      output_low(V_DROP); // high voltage operation
//    check_battery = 0;
//    }
  }



//-----------------------------------------------------------------------------
u16 get_reader_interrupts(u16 reader)
  {
  u16 x;
  x = 0;
  switch (reader)
    {
    case 0:
    if (input(MCR_CLOCK1)) // reader 0 D0
      {
      x |= 1;
      }
    if (input(MCR_DATA1 )) // reader 0 D1
      {
      x |= 2;
      }
    break;
    case 1:
    if (input(MCR_CLOCK2)) // reader 0 D0
      {
      x |= 1;
      }
    if (input(MCR_DATA2 )) // reader 0 D1
      {
      x |= 2;
      }
    break;
    case 2:
    if (input(MCR_CLOCK3)) // reader 0 D0
      {
      x |= 1;
      }
    if (input(MCR_DATA3 )) // reader 0 D1
      {
      x |= 2;
      }
    break;
    case 3:
    if (input(MCR_CLOCK4)) // reader 0 D0
      {
      x |= 1;
      }
    if (input(MCR_DATA4 )) // reader 0 D1
      {
      x |= 2;
      }
    break;
    }
  return x;
  }

//--------------------------------------------------------------------------
void check_fire_input(void)
  {
  if (analog_values[ANALOG_FIRE] > 2000) // no short on fire input?
    {
    if (fire_detected)
      {
      if (--fire_detected == 0)
        {
        send_fire_alarm(0);
        send_fire_cancel = 10;
        generate_event(0, 0, 0, EVT_fire_condition_ended);
        }
      }
    else
    fire_debounce = 0;
    }
  else  if (analog_values[ANALOG_FIRE] < 500)
    {
    if (fire_debounce < 100)
      {
      if (++fire_debounce >= 100)
        {
        generate_event(0, 0, 0, EVT_fire_condition);
        fire_detected = 100;
        send_fire_alarm(1);
        }
      }
    }
  }

//--------------------------------------------------------------------------
void check_tamper(void)
  {
  if (analog_values[ANALOG_TAMPER] > 500) // no short on tamper input?
    {
    if (!tamper_detected)
      {
      if (++tamper_debounce >= 30)
        {
        tamper_detected = 100;
        generate_event(0, 0, 0, EVT_tamper_detected);
        }
      }
    }
  else
    {
    if (!tamper_detected)
      {
      tamper_debounce = 0;
      }
    else
      {
      if (tamper_detected == 1000) // start condition?
        {
        tamper_detected = 0;
        }
      else if (--tamper_detected == 0)
        {
        generate_event(0, 0, 0, EVT_tamper_Restored);
        }
      }
    }
  }

//--------------------------------------------------------------------------
void clear_reader_outputs(void)
  {
  output_low(RDR_BUZZER1);
  output_low(RDR_BUZZER2);
  output_low(RDR_BUZZER3);
  output_low(RDR_BUZZER4);
  output_low(READER_LED1);
  output_low(READER_LED2);
  output_low(READER_LED3);
  output_low(READER_LED4);
  }


//--------------------------------------------------------------------------
void init_IOs(void)
  {
  output_drive(RDR_BUZZER1);
  output_drive(RDR_BUZZER2);
  output_drive(RDR_BUZZER3);
  output_drive(RDR_BUZZER4);
  output_drive(READER_LED1);
  output_drive(READER_LED2);
  output_drive(READER_LED3);
  output_drive(READER_LED4);
  output_drive(LED_RED);
  output_drive(LED_GREEN);
  output_drive(LOCK_RELAY1);
  output_drive(LOCK_RELAY2);
  output_drive(LOCK_RELAY3);
  output_drive(LOCK_RELAY4);
  output_drive(AUX_RELAY1);
  output_drive(AUX_RELAY2);
  output_drive(AUX_RELAY3);
  output_drive(AUX_RELAY4);
//  output_drive(ETH_CS);

  output_low(RDR_BUZZER1);
  output_low(RDR_BUZZER2);
  output_low(RDR_BUZZER3);
  output_low(RDR_BUZZER4);
  output_low(READER_LED1);
  output_low(READER_LED2);
  output_low(READER_LED3);
  output_low(READER_LED4);
  output_low(LED_RED);
  output_low(LED_GREEN);
  output_low(LOCK_RELAY1);
  output_low(LOCK_RELAY2);
  output_low(LOCK_RELAY3);
  output_low(LOCK_RELAY4);
  output_low(AUX_RELAY1);
  output_low(AUX_RELAY2);
  output_low(AUX_RELAY3);
  output_low(AUX_RELAY4);
//  output_low(ETH_CS);
    
  output_drive(VDROP);
  output_high(VDROP);
  }
