



/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"

#include "setup.h"
#include "serial.h"
#include "task_queue.h"
#include "circ_buff.h"
#include "serialTask.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

u8 Rx1Buffer[USART_IN_BUF_LEN];   // low level serial input buffer
u32 Rx1cnt = 0, Rx1in = 0, Rx1out = 0; // low level input/output pointers and byte counter
u32 Tx1Length;
u8 *Tx1BufPointer;
bool Tx1Transmitting;


u8 Rx2Buffer[USART_IN_BUF_LEN];   // low level serial input buffer
volatile u32 Rx2cnt = 0, Rx2in = 0, Rx2out = 0; // low level input/output pointers and byte counter
u32 Tx2Length;
u8 *Tx2BufPointer;
bool Tx2Transmitting, Tx1Transmitting;


u8 RS485_sending = 0;

extern u8 log[4096];
extern u32 logx;
extern bool Tx2Transmitting;
extern u32 Tx2Length;


//=============================================================================================
void UART1_receive_char(void)
  {
  u8 chr;
  if (USART1->SR & UART_FLAG_ORE)
    {
    USART1->SR &= ~UART_FLAG_ORE;
    }
  if (USART1->SR & UART_FLAG_NE)
    {
    USART1->SR &= ~UART_FLAG_NE;
    }
  if (USART1->SR & UART_FLAG_FE)
    {
    USART1->SR &= ~UART_FLAG_FE;
    }
  if (USART1->SR & UART_FLAG_PE)
    {
    USART1->SR &= ~UART_FLAG_PE;
    }

  if (USART1->SR & UART_FLAG_RXNE)
    {
    chr = USART1->DR;
    if (!RS485_sending)
    {
      Rx1Buffer[Rx1in] = chr;
      
      
//      FillBuffer(getHostCircBuffPtr(), &chr, 1);
      //UpdateWritePtr(getHostCircBuffPtr(), 1);
      
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
  }

//=============================================================================================
void putUsartChar(uint8_t uart, uint8_t ch)
  {
  if (uart == 1)
    {
    Rx1Buffer[Rx1in] = ch;
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
  else if(uart == 2)
    {
    Rx2Buffer[Rx2in] = ch;
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
  }

//=============================================================================================
void UART2_receive_char(void)
  {
  u8 chr;
  if (USART2->SR & UART_FLAG_ORE)
    {
    USART2->SR &= ~UART_FLAG_ORE;
    }
  if (USART2->SR & UART_FLAG_NE)
    {
    USART2->SR &= ~UART_FLAG_NE;
    }
  if (USART2->SR & UART_FLAG_FE)
    {
    USART2->SR &= ~UART_FLAG_FE;
    }
  if (USART2->SR & UART_FLAG_PE)
    {
    USART2->SR &= ~UART_FLAG_PE;
    }

  if (USART2->SR & UART_FLAG_RXNE)
    {
    chr = USART2->DR;
    if (!Tx2Transmitting)
      {
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
    }
  }

//=============================================================================================
void UART1_transmit_buffer(void)
  {
#pragma diag_suppress=Pe550
  u8 chr;
  if ((USART1->CR1 & USART_CR1_TXEIE) != 0 && (USART1->SR & UART_FLAG_TXE) != 0)
    {
    USART1->DR = *Tx1BufPointer++;
    if (--Tx1Length == 0 || !Tx1Transmitting)
      {
      __HAL_UART_DISABLE_IT(&huart1, UART_IT_TXE);
      USART1->SR &= ~UART_FLAG_TXE;
      Tx1Transmitting = false;
      chr = USART1->DR;
      }
    }
  }

//=============================================================================================
void UART2_transmit_buffer(void)
  {
  u8 chr;
  if (Tx2Length == 0 || Tx2Transmitting == 0)
    {
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_TXE);
    USART2->SR &= ~UART_FLAG_TXE;
    Tx2Transmitting = 0;
    Tx2Length = 0;
    chr = USART2->DR;
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    }
  else if ((USART2->CR1 & USART_CR1_TXEIE) != 0 && (USART2->SR & UART_FLAG_TXE) != 0)
    {
    USART2->DR = *Tx2BufPointer++;
    if (--Tx2Length == 0 || !Tx2Transmitting)
      {
      __HAL_UART_DISABLE_IT(&huart2, UART_IT_TXE);
      USART2->SR &= ~UART_FLAG_TXE;
      Tx2Transmitting = 0;
      Tx2Length = 0;
      chr = USART2->DR;
      __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
      }
    }
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
        if (--Rx1cnt == 0)                                              // then decrement
          {
          Rx1out = Rx1in;
          }
        }
      break;
    case 2:
      chr = Rx2Buffer[Rx2out];                                 // get next char
      if (++Rx2out >= USART_IN_BUF_LEN)                       // if output pointer overflows buffer length,
        {
        Rx2out = 0;                                           // then reset to beginning of buffer
        }
        if (--Rx2cnt == 0)                                              // then decrement
          {
          Rx2out = Rx2in;
          }
      break;
    }
  return chr;                                               // return character
  }

/*==============================================================
 * function:     send_char()
 * Description:  sends a single character to specified USART
 * Parameters:   message - a pointer to 8 bit character string to be sent
 * Returns:      NONE
===============================================================*/
void send_char(u32 channel, char chr)
  {
  switch (channel)
    {
    case 1:
//      USART1->DR = chr;
      HAL_UART_Transmit(&huart1, (u8 *)&chr, 1, 1);
      while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == 0);
      break;
    case 2:
//      USART2->DR = chr;
      HAL_UART_Transmit(&huart2, (u8 *)&chr, 1, 1);
      while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == 0);
      break;
    }
  }

/*==============================================================
 * function:     send_message()
 * Description:  sends a null terminated string to specified USART
 * Parameters:   message - a pointer to 8 bit character string to be sent
 * Returns:      NONE
===============================================================*/
void send_message(u32 channel, char *str)
  {
  u32 length;
  length = strlen(str);
  switch (channel)
    {
    case 1:
      HAL_UART_Transmit(&huart1, (u8 *)str, length, 100);
      break;
    case 2:
      HAL_UART_Transmit(&huart2, (u8 *)str, length, 100);
      break;
    }
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

//=============================================================================================
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
void ETHR_send_char(u8 chr)
  {
  //USART2->DR = chr;
  HAL_UART_Transmit(&huart2, &chr, 1, 1);
  while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET);
  delay_us(100);
  }

//=============================================================================
void RS485_send_char(u8 chr)
  {
//  USART1->DR = chr;
  HAL_UART_Transmit(&huart1, &chr, 1, 1);
  while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
  delay_us(100);
  }

