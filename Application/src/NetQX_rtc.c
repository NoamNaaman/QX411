/* Includes ------------------------------------------------------------------*/


/* USER CODE BEGIN Includes */

//#include "string.h"
#include "setup.h"
#include "main.h"

#include "setup.h"

extern RTC_HandleTypeDef hrtc;

#define START_WEEKDAY 5 /* Saturday Jan 1, 2000 */


//--------------------------------------------------------------------------
// Receives 8 bits from IýC device
// returns the received byte in the W reg.
//--------------------------------------------------------------------------


// convert a binary byte to 2 BCD digits
u8 RTC_conv_byte_to_BCD(u8 dat)
  {
  return ((dat / 10) << 4) | (dat % 10);
  }


// convert 2 BCD digits to a binary byte
//#separate
u8 RTC_conv_BCD_to_byte(u8 bcd)
  {
  return (bcd >> 4) * 10 + (bcd & 15);
  }


// convert binary date/time to BCD
//#separate
//void RTC_conv_dt_to_BCD(u8 *dt, u8 *bcd)
//  {
//  u16 cnt;
//  for (cnt = 0; cnt < 6; cnt++)
//    *bcd++ = RTC_conv_byte_to_BCD(*dt++);
//  }
//
//
//// convert BCD date/time to binary
////#separate
//void RTC_conv_BCD_to_dt(u8 *bcd, u8 *dt)
//  {
//  u8 buf[8];
//  u16 cnt;
//  for (cnt = 0; cnt < 6; cnt++, bcd++)
//    buf[cnt] = *dt++ = (*bcd >> 4) * 10 + (*bcd & 15);
//  buf[7] = 1;
//  }


// Compute day as number from year 0 day 1.
//#separate
u16 RTC_ComputeDay(DATE_TIME *dt)
  {
  u16 DayAcc = 0;                                                                       // clear day accumulator
  DayAcc  = dt->year * 365 + ((dt->year - 1) >> 2);                                      // multiply years by 365 and add a day for every four years
  DayAcc += CummulativeMonthDays[dt->month - 1] + dt->day;                               // add days of all months in current year up to current month
  if (dt->month > 2 && (dt->year & 3) == 0)                                              // if this year is a leap year and month is March to
    {
    DayAcc++;          
                                                                  // December add one day for February 29th
    }
  return DayAcc;                                                                         // return day accumulator
  }


// compute day of week
//#separate
u8 RTC_ComputeWeekDay(DATE_TIME *dt)
  {
  u16 WeekDay;
  WeekDay = RTC_ComputeDay(dt) + START_WEEKDAY;                                          // compute today's number and add
  return WeekDay % 7;                                                                    // 5. return day of week
  }

// check validity of date/time
u8 legal_date_time(DATE_TIME *dt)
  {
  if (dt->hour >= 24 || dt->minute >= 60 || dt->day > 31 || !dt->day || dt->month > 12 || !dt->month || dt->year > 50)
    {
    return 0;
    }
  return 1;
  }



// store converted date/time in RTC chip
//#separate
void RTC_store_date_time(DATE_TIME *idt)
  {
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  sTime.Hours   = idt->hour;
  sTime.Minutes = idt->minute;
  sTime.Seconds = idt->second;
  sTime.DayLightSaving = 0;
  sTime.TimeFormat = 0;
  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  sDate.Month = idt->month;
  sDate.Date  = idt->day;
  sDate.Year  = idt->year;
  sDate.WeekDay = RTC_ComputeWeekDay(idt);
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  }

#define __LL_RTC_CONVERT_BCD2BIN(__VALUE__) (uint8_t)(((uint8_t)((__VALUE__) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U + ((__VALUE__) & (uint8_t)0x0FU))

//=============================================================================================
void RTC_load_date_time(void)  
  {
  static u16  prev_hour, prev_minute;
  u32 xtime = 0; 
  u32 xdate = 0; 
  u32 Second = 0;
  u32 Minute = 0;
  u32 Hour   = 0;
  
  
  bool repeat = true;
  
  while (repeat)
    {
    xtime   = RTC->TR;
    xdate   = RTC->DR;
    
    Second  = xtime & 0x000000FF;
    Minute  = (xtime & 0x0000FF00) >> 8;
    Hour    = (xtime & 0x003F0000) >> 16;
    
    if (Hour != prev_hour)
      {
      if (Minute == 0 && prev_minute == 0x59)   
        {
        repeat = true;
        }
      else
        {
        repeat = false;
        }
      }
    else  
      {
      repeat = false;
      }
    prev_hour = Hour;
    prev_minute = Minute;
    }
    
    
  u32 Year    = (xdate & 0x00FF0000) >> 16;
  u32 Month   = (xdate & 0x00001F00) >> 8;
  u32 Day     = xdate & 0x000000FF;
  
  sys_date_time.second  = __LL_RTC_CONVERT_BCD2BIN(Second);
  sys_date_time.minute  = __LL_RTC_CONVERT_BCD2BIN(Minute);
  sys_date_time.hour    = __LL_RTC_CONVERT_BCD2BIN(Hour);
  sys_date_time.year    = __LL_RTC_CONVERT_BCD2BIN(Year);
  sys_date_time.month   = __LL_RTC_CONVERT_BCD2BIN(Month);
  sys_date_time.day     = __LL_RTC_CONVERT_BCD2BIN(Day);
  
  day_of_week = RTC_ComputeWeekDay(&sys_date_time);
  }

//=============================================================================================
// set hour:minute alarm in RTC chip
void RTC_set_alarm(u8 hour, u8 minute)
  {
  }


//=============================================================================================
// put RTC chip  in minute steady interrupt 2 mode
void RTC_set_alarm_cycle_minute(void)
  {
  }


//=============================================================================================
// stop alarm interrupts from RTC
void RTC_stop_alarm(void)
  {
  HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
  }


//=============================================================================================
// initialize RTC chip and establish communications
void RTC_init_chip(void)
  {
  HAL_RTC_Init(&hrtc);
  RTC_stop_alarm();                                                                      // do not generate any alarms
  }


//=============================================================================================
// get date/time from RTC and compute time in seconds from 00:00 1/1/2000
u32 RTC_compute_absolute_time(void)
  {
  DATE_TIME dt;
  u32 seconds;
  RTC_load_date_time();
  dt = sys_date_time;                                                               // get date/time from RTC
  seconds = (u32)(((u32)dt.year * 365L +
            CummulativeMonthDays[dt.month-1] + dt.day) * 1440L +
            (u32)dt.hour * 60L + dt.minute) * 60L + dt.second;                         // compute seconds
  return seconds;                                                                        // return time
  }

