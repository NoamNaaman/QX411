/* Includes ------------------------------------------------------------------*/
#include "stm32L1xx_hal.h"
#include "stm32L1xx.h"
#include "stm32L1xx_it.h"


#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "setup.h"
#include "lcd.h"
#include "serial.h"
#include "eeprom.h"
#include "misc.h"
#include "main.h"


char extra_line[22];
extern u16 button_pressed[4];
extern u32 motor_position, motor_version;

void MOTOR_steps(s32 offset);


typedef  struct {
  char Name[20];
  u8   line;
} MENU_DEF;

MENU_DEF  ANALOG_MENU[] = {
  { "Fixed Level", 1 },
  { "Ref analog",  2 },
  { "Analog Ops", 0 }
};

MENU_DEF  MAIN_MENU[] = {
  { "Set Parameters", 1 },
  { "Execute Steps",  2 },
  { "Analog Menu",    3 },
  { "Self Test",      4 },
  { "Cycle motor",    5 },
  { "Main Ops", 0 }
};

void  set_baseline(void);
void  set_5v_point(void);

void display_main_screen(void)
  {
  char buf[22];
  LCD_clearSCR();
  LCD_Position(0, 1); LCD_DspStr(MAIN_VERSION);
  if (motor_version < 100)
    {
    sprintf(buf, "Motor Ver. %d", motor_version);
    LCD_Position(0, 2); LCD_DspStr(buf);
    }

  delay_us(1000000);
  }

u32 get_number(u8 *head, u32 initial, u32 minx, u32 maxx, u32 step)
  {
  u32 sub_loop = 1, init = initial, ret = initial;
  u8 buf[11];
  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr(head);
  LCD_Position(0, 1); LCD_DspStr(extra_line);
  LCD_Position(0, 3); LCD_DspStr("B1/B2,B3=Save,B4=Next");
  while (1)
    {
    LCD_Position(0, 2);
    sprintf((char *)buf, "%5d", init);
    LCD_DspStr(buf);
    sub_loop = 1;
    while (sub_loop)
      {
      check_buttons();
      if (button_pressed[0])
        {
        button_pressed[0] = 0;
        if (init <= maxx - step)
          {
          init += step;
          }
        sub_loop = 0;
        }
      else if (button_pressed[1])
        {
        button_pressed[1] = 0;
        if (init >= minx + step)
          {
          init -= step;
          }
        sub_loop = 0;
        }
      else if (button_pressed[2])
        {
        button_pressed[2] = 0;
        LCD_Position(0, 0); LCD_DspStr(head); LCD_DspStr(" set");
        delay_ms(1000);
        ret = init;
        return ret;
        }
      else if (button_pressed[3])
        {
        button_pressed[3] = 0;
        return ret;
        }
      }
    }
  }

void set_parameters(void)
  {
  LCD_clearSCR();
  set_baseline();
  setup.topline = get_number("AMS Top", setup.topline, 500, 6000, 25);
  set_5v_point();
  setup.step_size = get_number("Step Size", setup.step_size, 10, 1000, 10);
  setup.deadband = get_number("AMS deadband", setup.deadband, 3, 50, 1);
  setup.baseline = get_number("AMS Bottom", setup.baseline, 0, 3250, 50);
  setup.delay = get_number("Vref delay", setup.delay, 1, 20, 1);

  strcpy(extra_line, "1=NC,2=NO,3=Latch");
  setup.power_fail_mode = get_number("Power fail", setup.power_fail_mode, 1, 3, 1);
  extra_line[0] = 0;

  write_setup();
  }

//void MOTOR_bottom(void)
//  {
////  MOTOR_relative(motor_position - setup.baseline + 35);
//  MOTOR_bottom();
//  }

void execute_steps(void)
  {
  u32 sub_loop = 1;
  s32 step, diff;
  u8 buf[22], onoff = 1;

      setup.automatic_mode = AUTO_STEP;
      write_setup();

  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Motor up/down   ON");
  LCD_Position(0, 3); LCD_DspStr("B1=U,B2=D,B4=Exit");
  while (1)
    {
    sub_loop = 1;
    while (sub_loop)
      {
      if (TMR_100mS_flags & 8)
        {
        TMR_100mS_flags &= ~8;
        MOTOR_wait_status();
        sprintf((char*)buf, "Position: %5d", motor_position - setup.baseline);
        LCD_Position(0, 2);
        LCD_DspStr(buf);
        }
      check_buttons();
      if (button_pressed[1])
        {
        button_pressed[1] = 0;
        if (!onoff)
          {
          continue;
          }
        step = setup.step_size;
        diff = motor_position - setup.baseline;
        if (diff < 8)
          {
          continue;
          }
        if (step > diff)
          {
          step = diff;
          if (step < 50)
            {
            step = 50;
            }
          }
        MOTOR_relative(step);
        sub_loop = 0;
        }
      else if (button_pressed[0])
        {
        button_pressed[0] = 0;
        if (!onoff)
          {
          continue;
          }
        if ((s32)motor_position - (s32)setup.baseline < (s32)setup.topline)
          {
          step = setup.step_size;
          if (abs(step) > 500)
            {
            __NOP();
            }
          MOTOR_relative(-step);
          }
        sub_loop = 0;
        }
      else if (button_pressed[2])
        {
        button_pressed[2] = 0;
        if (onoff)
          {
          onoff = 0;
          LCD_Position(0, 0); LCD_DspStr("Motor up/down   OFF");
          MOTOR_bottom();
          }
        else
          {
          onoff = 1;
          LCD_Position(0, 0); LCD_DspStr("Motor up/down   ON ");
          }
        sub_loop = 0;
        }
      else if (button_pressed[3]) // B4 pressed. exit
        {
        button_pressed[3] = 0;

      setup.automatic_mode = 0;
      write_setup();

        return;
        }
      }
    }
  }

float Vref, Vin;
s32 level, in1;
void get_voltages(void)
  {
  s32 diff;
  level = ADC_read(12);
  in1 = ADC_read(13);
  Vref = 9.0 * ((float)level / 3740.0);
  Vin = 9.0 * ((float)in1 / 3740.0);
  }

void execute_fixed_level(void)
  {
  s32 diff, l0, l1, rpos, interval = 10;
  float rel;
  u8 buf[22], onoff = 1;
  u16 sub_loop = 1;

      setup.automatic_mode = AUTO_AFIX;
      write_setup();

  level = setup.fixed_level;
  if (level >= 4095)
    {
    level = 2050;
    setup.fixed_level = level;
    write_setup();
    }
  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Fixed level     ON ");
  LCD_Position(0, 3); LCD_DspStr("B1=U,B2=D,B4=Exit");
  while (1)
    {
    while (sub_loop)
      {
      if (TMR_100mS_flags & 8)
        {
        TMR_100mS_flags &= ~8;
        MOTOR_wait_status();
        if (++interval >= 2)
          {
          interval = 0;
          get_voltages();
          rel = Vin / 5.0; // compute voltage/range
          l0 = setup.baseline;
          l1 = setup.topline + l0;
          rpos = (s32)((float)setup.pos5v * rel) + l0;
          sprintf((char *)buf, "F=%-5.2f", Vin);
          LCD_Position(0, 1); LCD_DspStr(buf);
          sprintf(buf, "AMS=%-4d Rpos=%-4d", motor_position - setup.baseline, rpos - setup.baseline);
          LCD_Position(0, 2); LCD_DspStr(buf);
          sprintf((char *)buf, "Diff=%-4d ", diff);
          LCD_Position(0, 3); LCD_DspStr(buf);
          diff = (s32)motor_position - rpos;
          if (onoff)
            {
            if (abs(diff) > setup.deadband)
              {
              l1 = abs(diff);
              if (l1 < 100)
                {
                l1 = 100;
                }
              if (diff > 0)
                {
                MOTOR_relative(l1 / 2);
                }
              else
                {
                MOTOR_relative(-l1 / 2);
                }
              }
            }
          }
        }
      check_buttons();
      if (button_pressed[0])
        {
        button_pressed[0] = 0;
        if (level <= 4000)
          {
          level += 50;
          }
        }
      else if (button_pressed[1])
        {
        button_pressed[1] = 0;
        if (level >= 50)
          {
          level -= 50;
          }
        }
      else if (button_pressed[2])
        {
        button_pressed[2] = 0;
        if (onoff)
          {
          MOTOR_bottom();
//          MOTOR_relative(motor_position - setup.baseline);// + setup.step_size);
          onoff = 0;
          LCD_Position(0, 0); LCD_DspStr("Fixed level     OFF");
          }
        else
          {
          onoff = 1;
          LCD_Position(0, 0); LCD_DspStr("Fixed level     ON ");
          }
        }
      else if (button_pressed[3]) // B4 pressed. exit
        {
        button_pressed[3] = 0;

      setup.automatic_mode = 0;
      write_setup();

        return;
        }
      }
    }
  }

// measure two analog inputs. 3740==9V
u16 analog_steps[64];
u32 astx = 0;




void execute_differential(void)
  {
  s32 diff, add = 0, delay = 3;
  u32 wait_finish = 0;
  u32 skip = 10, skip_limit;
  float ratio, first = 0.2;
  u8 buf[22], finish, onoff = 1;

      setup.automatic_mode = AUTO_AREF;
      write_setup();

  astx = 0;

  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Differential  ON");
  LCD_Position(0, 3); LCD_DspStr("B4=Exit");
  while (1)
    {
    while (1)
      {
      if (TMR_100mS_flags & 8)
        {
        TMR_100mS_flags &= ~8;
        skip_limit = setup.delay;
        if (!onoff)
          {
          skip_limit = 1;
          }
        if (++skip >= skip_limit)
          {
          skip = 0;
          get_voltages();
          if (!onoff)
            {
            in1 = 0;
            }
          diff = abs(in1 - level);// / 10;
          ratio = (float)diff / 4096.0;
          diff = (s32)((float)setup.topline * ratio * first);
          if (first < 1.0)
            {
            first += 0.05;
            }
//          delay = diff / 200 + 3;
          sprintf((char *)buf, "Vin=%4.2f  Vref=%4.2f ", Vin, Vref);
          LCD_Position(0, 1); LCD_DspStr(buf);
          sprintf((char *)buf, "Mdiff=%-4d ", diff);
          LCD_Position(0, 2); LCD_DspStr(buf);
          MOTOR_wait_status();
          sprintf((char *)buf, "pos=%-5d ", motor_position - setup.baseline);
          LCD_Position(0, 3); LCD_DspStr(buf);

          wait_finish = 0;
          if (onoff && diff > setup.deadband)
            {
            analog_steps[astx++ & 63] = diff;
            analog_steps[astx & 63] = 0xFFFF;
            if (diff < 40)
              {
              diff = 40;
              }
            if (in1 > level)
              {
              if (motor_position - setup.baseline + diff > setup.topline)
                {
                diff /= 2;
                }
              MOTOR_relative(-diff);
              wait_finish = 1;
              }
            else
              {
              MOTOR_relative(diff);
              wait_finish = 1;
              }
            }
          if (wait_finish)
            {
            wait_finish = 0;
            while (MOTOR_rx_status() == 0);
            finish = MOTOR_get_char();
            }
          }
        }
      check_buttons();
      if (button_pressed[2]) // B3 pressed. toggle on/off
        {
        button_pressed[2] = 0;
        if (onoff)
          {
          onoff = 0;
          MOTOR_bottom();
//          MOTOR_relative(motor_position - setup.baseline);// + setup.step_size);
//          MOTOR_relative(100);
          LCD_Position(0, 0); LCD_DspStr("Differential  OFF");
          delay_ms(1000);
          }
        else
          {
          onoff = 1;
          LCD_Position(0, 0); LCD_DspStr("Differential  ON ");
          add = 200;
          first = 0.4;
          astx = 0;
          }
        }
      if (button_pressed[3]) // B4 pressed. exit
        {
        button_pressed[3] = 0;

      setup.automatic_mode = 0;
      write_setup();

        return;
        }
      }
    }
  }

void save_baseline(void)
  {
  char buf[22];
  setup.baseline = motor_position;
  write_setup();
  sprintf(buf, "Baseline set to %d", setup.baseline);
  LCD_clearSCR();
  LCD_Position(0, 2);
  LCD_DspStr(buf);
  delay_ms(1200);
  }

void set_baseline(void)
  {
  char buf[22];
  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Set Baseline");
  LCD_Position(0, 3); LCD_DspStr("B1/B2,B3=Set,B4=Next");
  while (1)
    {
    while (1)
      {
      check_buttons();
      if (TMR_100mS_flags & 8)
        {
        TMR_100mS_flags &= ~8;
        MOTOR_wait_status();
        sprintf((char *)buf, "pos=%-5d ", motor_position);
        LCD_Position(0, 2); LCD_DspStr(buf);
        if (button_pressed[1])
          {
          button_pressed[1] = 0;
          MOTOR_relative(50);
          }
        else if (button_pressed[0])
          {
          button_pressed[0] = 0;
          MOTOR_relative(-50);
          }
        else if (button_pressed[2])
          {
          button_pressed[2] = 0;
          save_baseline();
          return;
          }
        else if (button_pressed[3])
          {
          button_pressed[3] = 0;
          return;
          }
        }
      }
    }
  }

u32 menu_item(MENU_DEF *menu, u32 first_line)
  {
  u32 sub_loop = 1, lines, curpos = first_line;
  control_led(2, 0);
  for (lines = 0; menu[lines].line; lines++); // count lines in menu
  while (1)
    {
    control_led(2, 0);
    LCD_clearSCR();
    LCD_Position(0, 0); LCD_DspStr((u8 *)menu[lines].Name);
    LCD_Position(0, 3); LCD_DspStr("B1=D,B3=Exit,B4=Sel");
    if (curpos == lines) // on last line of menu list?
      {
      LCD_Position(2, 1); LCD_DspStr((u8 *)menu[lines-2].Name);
      LCD_Position(2, 2); LCD_DspStr((u8 *)menu[lines-1].Name);
      LCD_Position(0, 2); LCD_DspStr(">>");
      }
    else
      {
      LCD_Position(2, 1); LCD_DspStr((u8 *)menu[curpos-1].Name);
      LCD_Position(2, 2); LCD_DspStr((u8 *)menu[curpos].Name);
      LCD_Position(0, 1); LCD_DspStr(">>");
      }

    sub_loop = 1;
    while (sub_loop)
      {
      check_buttons();
      PC_handle_comm();
      if (button_pressed[0])
        {
        button_pressed[0] = 0;
        if (++curpos > lines)
          {
          curpos = 1;
          }
        sub_loop = 0;
        }
      else if (button_pressed[2])
        {
        button_pressed[2] = 0;
        return 0;
        }
      else if (button_pressed[3])
        {
        button_pressed[3] = 0;
        return curpos;
        }
      }
    }
  }

void analog_menu(void)
  {
  u32 select = 1;
  while (1)
    {
    select = menu_item(&ANALOG_MENU[0], select);
    switch (select)
      {
      case 0:
        return;
      case 1:
        execute_fixed_level();
        break;
      case 2:
        execute_differential();
        break;
      default:
        select = 1;
        break;
      }
    }
  }

void set_5v_point(void)
  {
  u8 buf[22], finish;
  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Set 5V point");
  LCD_Position(0, 3); LCD_DspStr("B3=Save,B4=Next");
  while (1)
    {
    get_voltages();
    check_buttons();
    PC_handle_comm();
    if (TMR_100mS_flags & 8)
      {
      TMR_100mS_flags &= ~8;
      MOTOR_wait_status();
      sprintf((char *)buf, "Vin=%-5.2f pos=%-5d ", Vin, motor_position - setup.baseline);
      LCD_Position(0, 2); LCD_DspStr(buf);
      if (button_pressed[1])
        {
        button_pressed[1] = 0;
        MOTOR_relative(50);
        }
      else if (button_pressed[0])
        {
        button_pressed[0] = 0;
        MOTOR_relative(-50);
        }
      else if (button_pressed[2])
        {
        button_pressed[2] = 0;
        setup.pos5v = motor_position - setup.baseline;
        write_setup();
        LCD_clearSCR();
        LCD_Position(0, 2); LCD_DspStr("5V point saved");
        delay_ms(1200);
        return;
        }
      else if (button_pressed[3])
        {
        button_pressed[3] = 0;
        return;
        }
      }
    }
  }

void self_test(void)
  {
  u8 buf[22], finish;
  u32 stat, missing = 0;
  LCD_clearSCR();
  LCD_Position(0, 0); LCD_DspStr("Self Test");
  LCD_Position(0, 3); LCD_DspStr("B1=U,B2=D,B3=SB,B4=X");
  while (1)
    {
    if (TMR_100mS_flags & 8)
      {
      TMR_100mS_flags &= ~8;
      get_voltages();
      stat = MOTOR_wait_status();
      sprintf((char *)buf, "Vin=%4.2f  Vref=%4.2f ", Vin, Vref);
      LCD_Position(0, 1); LCD_DspStr(buf);
      if (!stat && ++missing > 5)
        {
        strcpy(buf, "MOTOR board missing");
        }
      else
        {
        missing = 0;
        sprintf((char *)buf, "MOTOR pos=%-10d", motor_position);
        }
      LCD_Position(0, 2); LCD_DspStr(buf);
      }
    check_buttons();
    if (button_pressed[1])
      {
      button_pressed[1] = 0;
      MOTOR_steps(setup.step_size);
      }
    else if (button_pressed[0])
      {
      button_pressed[0] = 0;
      MOTOR_steps(-(s16)setup.step_size);
      }
    else if (button_pressed[2])
      {
      button_pressed[2] = 0;
      save_baseline();
      set_parameters();
      return;
      }
    else if (button_pressed[3])
      {
      button_pressed[3] = 0;
      return;
      }
    }
  }

void cycle_motor(void)
  {
  char buf[22];
  s32 cycles = 10, step_size = 250, delay = 1500, loop, mloop, updown, steps, mdelay;
  cycles =     get_number("Cycles",    setup.cycle_repeat,  1, 1000, 1);
  steps =      get_number("Steps",     setup.cycle_steps ,  1, 10, 1);
  step_size =  get_number("Step size", setup.cycle_step_size,    30, 500, 10);
  delay =      get_number("Delay",     setup.cycle_delay,   250, 3000, 10);
  updown = 500;
  if ((setup.cycle_repeat  != cycles) ||
      (setup.cycle_steps != steps) ||
      (setup.cycle_step_size != step_size) ||
      (setup.cycle_delay   != delay))
    {
    setup.cycle_repeat = cycles;
    setup.cycle_steps = steps;
    setup.cycle_step_size = step_size;
    setup.cycle_delay = delay;
    write_setup();
    }

  LCD_clearSCR();

  for (mloop = cycles; mloop; mloop--)
    {
    LCD_Position(5, 2);
    sprintf(buf, "Cycles left %d", mloop);
    LCD_DspStr(buf);
    for (loop = 0; loop < steps; loop++)
      {
      check_buttons();
      updown += step_size;
      MOTOR_absolute(updown);
      sprintf(buf, "Position %d", motor_position);
      LCD_Position(5, 3);
      LCD_DspStr(buf);
      for (mdelay = 0; mdelay < delay; mdelay++)
        {
        delay_ms(1);
        check_buttons();
        if (button_pressed[3])
          {
          button_pressed[3] = 0;
          mloop = 1;
          }
        }
      }
    delay_ms(1000);
    for (loop = 0; loop < steps; loop++)
      {
      check_buttons();
      updown -= step_size;
      MOTOR_absolute(updown);
      for (mdelay = 0; mdelay < delay; mdelay++)
        {
        delay_ms(1);
        check_buttons();
        if (button_pressed[3])
          {
          button_pressed[3] = 0;
          mloop = 1;
          }
        }
      }
    delay_ms(1000);
    }
  }

void main_menu(void)
  {
  u32 select = 1;
  while (1)
    {
    control_led(2, 0);
    select = menu_item(&MAIN_MENU[0], select);
    switch (select)
      {
      case 1:
        set_parameters();
        break;
      case 2:
        output_high(EN50V);
        execute_steps();
        break;
      case 3:
        output_high(EN50V);
        analog_menu();
        break;
      case 4:
        output_high(EN50V);
        self_test();
        break;
      case 5:
        output_high(EN50V);
        cycle_motor();
      default:
        select = 1;
        break;
      }
    output_low(EN50V);
    }
  }


