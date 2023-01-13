/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "setup.h"
#include "HND_lcd.h"
#include "serial.h"
#include "eeprom.h"
//#include "misc.h"


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
  char buf[66];
  sprintf(buf, "Lockers %d", SW_VERSION);
  LCD_clearSCR();
  LCD_Position(0, 1); 
  LCD_DspStr(buf);
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
        break;
      case 3:
        break;
      case 4:
        break;
      case 5:
      default:
        break;
      }
    }
  }


