#ifndef __LCD
#define __LCD

void LCD_mode(u8 mode);
void LCD_shift(u8 RTlf);
void LCD_clearSCR(void);
void init_LCD(void);
void LCD_DspChar(u8 chr);
u32 LCD_DspStr(u8 *str);
void LCD_Dsp2digits(u8 num);
void LCD_Dsp4digits(u16 num);
void LCD_Position(u8 x, u8 y);
void LCD_Home(void);
void LCD_clearEOL(void);
void LCD_backlight(u32 onoff);

extern u32  backlight_counter;

#endif