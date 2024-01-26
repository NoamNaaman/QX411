#include "main.h"

#include "setup.h"

u16 t100us = 0, prev_rdr2_bits;

/*
2 - magnetic card reader
3 - Wiegand output device
*/
u16 characters = 0;
//u8 testbuf[256], tbx = 0;

u32 default_ictr = 0;

extern u32 rdr4_receiving[MAX_DOORS];
extern u16 rdr4_xmt;

void rdr4_init_char_rx(u32 reader);
void rdr4_send_bit_stream(void);
void rdr4_receive_byte(u32 reader);
void rdr4_init_char_rx(u32 reader);
void rdr4_receive_bit(void);

//--------------------------------------------------------------------------
void UserSysTick_Handler(void) // 1ms
  {
//  u16 inp;
  t100us = 0;
  Timer_1mS_Flags = 0xFFFFFFFF;
  Timer_1mS_Count[0]++;
  Timer_1mS_Count[1]++;
  Timer_1mS_Count[2]++;
  Timer_1mS_Count[3]++;
  comm_timer++;
  if (eeprom_write_timer)
    {
    eeprom_write_timer--;
    }
  if (!Timer_1ms_counter || Timer_1ms_counter == 5)
    {
    Timer_5mS_Flags = 0xFFFFFFFF;
    }  
  if (++Timer_1ms_counter >= 10)
    {
    Timer_1ms_counter = 0;
    Timer_10ms_count[0]++;
    Timer_10ms_count[1]++;
    Timer_10mS_Flags = 0xFFFFFFFF;
    if (++Timer_10mS_Cnt >= 10)
      {
      Timer_10mS_Cnt = 0;
      Timer_100mS_Flags =  0xFFFFFFFF;
      }
    }
  }
  
//--------------------------------------------------------------------------
// reader interface
void WD0_or_MCRclk_handler(u32 reader) // reader RFID data or Wiegand D0 or MCR CLK
  {
  if (doors[reader].Reader_type == RDR_MAGCARD)
    {
    MCR_MagData(reader);
    }
  else
    {
    wiegand_d0(reader);
    }
  }


//--------------------------------------------------------------------------
// reader interface
void WD1_handler(u32 reader) // reader Wiegand D1
  {
  if (rdr4_active)//doors[reader].Reader_type == RDR_RDR4)
    {
    if (!rdr4_receiving[reader])
      {
      rdr4_init_char_rx(reader);
      }
    }
  else if (doors[reader].Reader_type == RDR_WIEGAND)
    {
    wiegand_d1(reader);
    }
  }


////--------------------------------------------------------------
//
//void  RS485_receive_char(void)
//  {
//  u16 x;
//  x = U1RXREG;
//  COM1_rbuf[COM1_rxi & (COM1_RX_LEN-1)] = x;
//  if (++COM1_rxi >= COM1_RX_LEN)
//    COM1_rxi = 0;
//  if (COM1_rcnt < COM1_RX_LEN)
//    COM1_rcnt++;
//  characters++;
////  testbuf[tbx++] = x;
//  }
//
////--------------------------------------------------------------------------
///*
// * Ethernet module interface
// * Assumes that if ethernet is active, RS485 is not.
// * Uses same channel buffers as RS485.
// *
// */
//u16 prev_eth_char = 0;
//void  XPORT_receive_char(void)
//  {
//  u16 x;
//  x = U4RXREG;
//  COM1_rbuf[COM1_rxi & (COM1_RX_LEN-1)] = x;
//  if (++COM1_rxi >= COM1_RX_LEN)
//    COM1_rxi = 0;
//  if (COM1_rcnt < COM1_RX_LEN)
//    COM1_rcnt++;
//  characters++;
//  if (prev_eth_char == PREAMBLE0 && x == PREAMBLE1)
//    Ethernet_active = 12345;
//  prev_eth_char = x;
//  }
//
//

//=============================================================================================
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
  {
  if (GPIO_Pin == RDR1_D0_INTR)//GPIO_PIN_15)        
    {    // R1D0                      
    delay_us(2);
    if (input(RDR1_D0) == 1)
      {
      delay_us(2);
      if (input(RDR1_D0) == 1)
        {
        WD0_or_MCRclk_handler(0);     
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR1_D0) == 1);
          }
        }
      }
    }    
  
  if (GPIO_Pin == RDR1_D1_INTR)//GPIO_PIN_10)        
    {     // R1D1                      
    delay_us(2);
    if (input(RDR1_D1) == 1)
      {
      delay_us(2);
      if (input(RDR1_D1) == 1)
        {
        WD1_handler(0); // handle Wiegand D1 or RDR4 RX
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR1_D1) == 1);
          }
        }
      }
    }
  
  if (GPIO_Pin == GPIO_PIN_0)
    {    // R2D0
    delay_us(2);
    if (input(RDR2_D0) == 1)
      {
      delay_us(2);
      if (input(RDR2_D0) == 1)
        {
        WD0_or_MCRclk_handler(1);
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR2_D0) == 1);
          }
        }
      }
    }
  
  if (GPIO_Pin == GPIO_PIN_8)
    {    // R2D1
    delay_us(2);
    if (input(RDR2_D1) == 1)
      {
      delay_us(2);
      if (input(RDR2_D1) == 1)
        {
        WD1_handler(1); // handle Wiegand D1 or RDR4 RX
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR2_D1) == 1);
          }
        }
      }
    }
  
  if (GPIO_Pin == GPIO_PIN_14)
    {    // R3D0
    delay_us(2);
    if (input(RDR3_D0) == 1)
      {
      delay_us(2);
      if (input(RDR3_D0) == 1)
        {
        WD0_or_MCRclk_handler(2);
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR3_D0) == 1);
          }
        }
      }
    }

  if (GPIO_Pin == GPIO_PIN_13)
    {    // R3D0
    delay_us(2);
    if (input(RDR3_D1) == 1)
      {
      delay_us(2);
      if (input(RDR3_D1) == 1)
        {
        WD1_handler(2); // handle Wiegand D1 or RDR4 RX
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR3_D1) == 1);
          }
        }
      }
    }
  
  if (GPIO_Pin == GPIO_PIN_7)
    {    // R4D0
    delay_us(2);
    if (input(RDR4_D0) == 1)
      {
      delay_us(2);
      if (input(RDR4_D0) == 1)
        {
        WD0_or_MCRclk_handler(3);
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR4_D0) == 1);
          }
        }
      }
    }
    
  if (GPIO_Pin == GPIO_PIN_9)
    {    // R4D1
    delay_us(2);
    if (input(RDR4_D1) == 1)
      {
      delay_us(2);
      if (input(RDR4_D1) == 1)
        {
        WD1_handler(3); // handle Wiegand D1 or RDR4 RX
        if (!rdr4_active)
          { // wait for Wiegand pulse to be over
          while (input(RDR4_D1) == 1);
          }
        }
      }
    }
  }

//=============================================================================================
void enable_interrupt_pin(u32 pin)
  {
  EXTI->IMR |= pin;
  }

//=============================================================================================
void disable_interrupt_pin(u32 pin)
  {
  EXTI->IMR &= ~pin;
  }

//=============================================================================================
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
  {
  if (htim->Instance == TIM1)
    {
//    output_high(ETH_RST_RS485EN);
    CheckWiegandRdrD1b();
//    output_low(ETH_RST_RS485EN);
    }
  
  if (htim->Instance == TIM4)
    {
    if (rdr4_xmt)
      {
      rdr4_send_bit_stream();
      }
    }
  else if (htim->Instance == TIM5)
    {
    rdr4_receive_bit();
    }
  }

