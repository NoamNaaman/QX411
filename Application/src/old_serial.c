/* Includes ------------------------------------------------------------------*/

#include "main.h"


#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"

#include "setup.h"
#include "serial.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;


u8 Rx1Buffer[USART_IN_BUF_LEN];   // low level serial input buffer
u32 Rx1cnt = 0, Rx1in = 0, Rx1out = 0; // low level input/output pointers and byte counter

u8 Rx2Buffer[USART_IN_BUF_LEN];   // low level serial input buffer
u32 Rx2cnt = 0, Rx2in = 0, Rx2out = 0; // low level input/output pointers and byte counter

u8 RS485_sending = 0;

extern u8 log[4096];
extern u32 logx;

void UART1_receive_char(void)
  {
  u8 chr;
  if (USART1->SR & UART_FLAG_ORE)
    {
    USART1->CR |= UART_FLAG_ORE;
    }
  if (USART1->SR & UART_FLAG_NE)
    {
    USART1->CR |= UART_FLAG_NE;
    }
  if (USART1->SR & UART_FLAG_FE)
    {
    USART1->CR |= UART_FLAG_FE;
    }
  if (USART1->SR & UART_FLAG_PE)
    {
    USART1->CR |= UART_FLAG_PE;
    }

  if (USART1->SR & UART_FLAG_RXNE)
    {
    chr = USART1->RDR;
    if (!RS485_sending)
      {
      Rx1Buffer[Rx1in] = chr;
      if (++Rx1in >= USART_IN_BUF_LEN)
        {
        Rx1in = 0;
        }
      if (Rx1cnt < USART_IN_BUF_LEN)
        {
        Rx1cnt++;
        }
      else
        {
        Rx1out = Rx1in;
        }
      }
    }
  USART1->RQR |= UART_RXDATA_FLUSH_REQUEST;
  }

void UART2_receive_char(void)
  {
  u8 chr;
  if (USART2->SR & UART_FLAG_ORE)
    {
    USART2->CR |= UART_FLAG_ORE;
    }
  if (USART2->SR & UART_FLAG_NE)
    {
    USART2->CR |= UART_FLAG_NE;
    }
  if (USART2->SR & UART_FLAG_FE)
    {
    USART2->CR |= UART_FLAG_FE;
    }
  if (USART2->SR & UART_FLAG_PE)
    {
    USART2->CR |= UART_FLAG_PE;
    }

  if (USART2->SR & UART_FLAG_RXNE)
    {
    chr = USART2->RDR;
    Rx2Buffer[Rx2in] = chr;
    if (++Rx2in >= USART_IN_BUF_LEN)
      {
      Rx2in = 0;
      }
    if (Rx2cnt < USART_IN_BUF_LEN)
      {
      Rx2cnt++;
      }
    else
      {
      Rx2out = Rx2in;
      }
    }
  USART2->RQR |= UART_RXDATA_FLUSH_REQUEST;
  }



/*==============================================================
 * function:     get_usart_byte()
 * Description:  returns oldest character in receive buffer
 * Parameters:   NONE
 * Returns:      oldest received character
===============================================================*/
u16 get_usart_byte(u32 channel)
  {
  u16 chr = 0;
  switch (channel)
    {
    case 1:
      chr = Rx1Buffer[Rx1out];                                 // get next char
      if (++Rx1out >= USART_IN_BUF_LEN)                       // if output pointer overflows buffer length,
        {
        Rx1out = 0;                                           // then reset to beginning of buffer
        }
      if (Rx1cnt)                                              // if (counter is greater than 0,
        {
        Rx1cnt--;                                              // then decrement
        }
      break;
    case 2:
      chr = Rx2Buffer[Rx2out];                                 // get next char
      if (++Rx2out >= USART_IN_BUF_LEN)                       // if output pointer overflows buffer length,
        {
        Rx2out = 0;                                           // then reset to beginning of buffer
        }
      if (Rx2cnt)                                              // if (counter is greater than 0,
        {
        Rx2cnt--;                                              // then decrement
        }
      break;
    }
  return chr;                                               // return character
  }

/*==============================================================
 * function:     send_message()
 * Description:  sends a null terminated string to specified USART
 * Parameters:   message - a pointer to 8 bit character string to be sent
 * Returns:      NONE
===============================================================*/
void send_message(char *str)
  {
  u32 length;
  length = strlen(str);
  HAL_UART_Transmit(&huart2, (u8 *)str, length, 100);
  }

/*==============================================================
 * function:     send_block()
 * Description:  sends a null terminated string to specified USART
 * Parameters:   message - a pointer to a byte block to be sent
 * Returns:      NONE
===============================================================*/
void send_block(u32 length, u8 *message)
  {
  HAL_UART_Transmit(&huart2, (u8 *)message, length, 100);
  }

/*==============================================================
 * function:     get_comm_buffer_status()
 * Description:  returns number of received characters waiting in buffer
 * Parameters:   NONE
 * Returns:      character count for specified channel
===============================================================*/
u32 get_comm_buffer_status(u32 channel)
  {
  switch (channel)
    {
    case 1:
      return Rx1cnt;
    case 2:
      return Rx2cnt;
    }
  return 0;
  }

////=================================================================  
//void send_RS485_message(char *str)
//  {
//  u32 length;
//  length = strlen(str);
////  __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
//  output_high(RS485_EN);
//  delay_us(5);
//  RS485_sending = 1;
//  while (length--)
//    {
//    log[logx++ & 4095] = *str;
//    huart1.Instance->TDR = *str++;
//    delay_us(105);
//    }
////  HAL_UART_Transmit(&huart1, (u8 *)str, length, 100);
//  delay_us(5);
//  output_low(RS485_EN);
//  RS485_sending = 0;
////  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
//  }

//=================================================================  
void flush_rx_buffer(u32 channel)
  {
  switch (channel)
    {
    case 1:
      Rx1cnt = 0; Rx1in = 0; Rx1out = 0; // low level input/output pointers and byte counter
      break;
    case 2:
      Rx2cnt = 0; Rx2in = 0; Rx2out = 0; // low level input/output pointers and byte counter
     break;
    }
  }

//=============================================================================
void comm_reinit(void)
  {
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
  }

//=============================================================================
void ETHR_send_char(u8 chr)
  {
  USART2->TDR = chr;
  while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET);
  delay_us(100);
  }

//=============================================================================
void RS485_send_char(u8 chr)
  {
  USART1->TDR = chr;
  while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
  delay_us(100);
  }

//=============================================================================
void ALL_send_char(u8 chr)
  {
  USART2->TDR = chr;
  USART1->TDR = chr;
  while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET);
  }

//=============================================================================
