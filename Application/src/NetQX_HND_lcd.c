/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "setup.h"
#include "HND_lcd.h"

#define LCD_CHARACTERS 20
#define LCD_LINES 4


u8 LCD_Cursor, LCD_x, LCD_y;
u8 LCD_image[LCD_LINES][LCD_CHARACTERS];
u32  backlight_counter = 0;

#define LCD_RS   GPIOD,1
#define LCD_E    GPIOE,6
#define LCD_DB4  GPIOC,13
#define LCD_DB5  GPIOD,9
#define LCD_DB6  GPIOD,15
#define LCD_DB7  GPIOC,7


void LCD_Position(u8 x, u8 y);

//==================================================================-
void LCD_nibble(u8 nibble)
  {
  set_output(LCD_DB4, nibble & 1);
  set_output(LCD_DB5, nibble & 2);
  set_output(LCD_DB6, nibble & 4);
  set_output(LCD_DB7, nibble & 8);
  delay_us(2);
  output_high(LCD_E);
  delay_us(2);
  output_low(LCD_E);
  }

//==================================================================-
void write_LCD(u8 datab, u8 data_ctl)
  {
  set_output(LCD_RS, data_ctl);
  LCD_nibble(datab >> 4);
  LCD_nibble(datab & 0x0F);
  delay_us(60);
  }

//==================================================================-
void LCD_mode(u8 mode)
  {
  u8 mask;
  mask = 8 | mode;
  write_LCD(mask, 0); // display clear
  delay_ms(3);
  }

//==================================================================-
void LCD_shift(u8 RTlf)
  {
  u8 wrd;
  wrd = 0x10;
  if (RTlf)
    wrd |= 4;
  write_LCD(wrd,0); // display clear
  delay_us(50);
  }

//==================================================================-
void LCD_clearSCR(void)
  {
  write_LCD(0x01,0); // display clear
  delay_us(3000);
  LCD_Cursor = 0;
  LCD_x = 0;
  LCD_y = 0;
  memset(LCD_image, ' ', sizeof(LCD_image));
  }

//==================================================================-
void init_LCD(void)
  {
//  u8 idx;
  output_low(LCD_RS);
//  output_low(LCD_RW);
  output_low(LCD_E);
  delay_ms(20);
  LCD_nibble(3);
  delay_ms(5);
  LCD_nibble(3);
  delay_ms(5);
  LCD_nibble(3);
  delay_ms(5);
  LCD_nibble(2);
  delay_ms(5);
  memset(LCD_image, ' ', sizeof(LCD_image));
  write_LCD(0x28,0); // two lines
  delay_ms(5);
  write_LCD(0x10,0); // two lines
  delay_us(100);
  write_LCD(0x0F,0); // 0x0C=display on, cursor off, blink off
  delay_us(100);
  write_LCD(0x06,0); // 0x0C=display on, cursor off, blink off
  delay_us(100);
  LCD_clearSCR();
  }

//==================================================================-
void LCD_DspChar(u8 chr)
  {
  if (LCD_x >= LCD_CHARACTERS || LCD_y >= LCD_LINES)
    return;
  write_LCD(0x80 + (LCD_Cursor & 0x7F), 0);
  delay_us(100);
  write_LCD(chr, 1);
  delay_us(100);
  LCD_Cursor++;
  LCD_image[LCD_y][LCD_x++] = chr; // update LCD image and advance X
//  backlight_counter = 100;
//  LCD_backlight(1);
  }

//==================================================================-
u32 LCD_DspStr(u8 *str)
  {
  u32 newline;
  newline = 0;
  while (*str)
    {
    if (*str == '\n')
      {
      LCD_y++;
      LCD_x = 0;
      LCD_Position(LCD_x, LCD_y);
      str++;
      newline = 1;
      }
    else
      LCD_DspChar(*str++);
    }
  return newline;
  }

//==================================================================-
void LCD_Dsp2digits(u8 num)
  {
  LCD_DspChar((num / 10) + '0');
  LCD_DspChar((num % 10) + '0');
  }

//==================================================================-
void LCD_Dsp4digits(u16 num)
  {
  LCD_Dsp2digits(num / 100);
  LCD_Dsp2digits(num % 100);
  }

//==================================================================-
void LCD_Position(u8 x, u8 y)
  {
  switch (y)
    {
    case 0: LCD_Cursor = 0;    break;// 00
    case 1: LCD_Cursor = 0x40; break;// 40
    case 2: LCD_Cursor = 0x14; break;// 14
    case 3: LCD_Cursor = 0x54; break;// 54
    }
  LCD_Cursor += x;
  LCD_x = x;
  LCD_y = y;
  }

//==================================================================-
void LCD_Home(void)
  {
  LCD_Position(0,0);
  }

//==================================================================-
void LCD_clearEOL(void)
  {
  u8 org_Cursor;
  org_Cursor = LCD_Cursor;
  while (LCD_Cursor++ & 15)
    LCD_DspChar(' ');
  LCD_Cursor = org_Cursor;
  }

//==================================================================-
//void LCD_backlight(u32 onoff)
//  {
//  set_output(LCD_BL, onoff != 0);
//  backlight_counter = 100;
//  }
