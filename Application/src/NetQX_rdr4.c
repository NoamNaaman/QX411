#include "main.h"

#include "setup.h"

u8  RDR4_rbuf[MAX_DOORS][COM1_RX_LEN];
u32 RDR4_rcnt[MAX_DOORS];
u32  RDR4_rxi[MAX_DOORS];
u32  RDR4_rxo[MAX_DOORS];

u32 rdr4_cmd[10];
u32 rdr4_cmd_len;
u32 rdr4_cmd_idx, rdr4_cmd_bitx, rdr4_cmd_ch;
u32 rdr4_xmt, rdr4_cmd_curw, rdr4_query;

u32  rdr4_reply_bitx, rdr4_reply_bytex;
u8   rdr4_reply[MAX_DOORS][16];
u32  rdr4_reply_idx[MAX_DOORS];
u32  rdr4_rx_state[MAX_DOORS];
u32  rdr4_latest_state[MAX_DOORS] = {0xF2,0xF2,0xF2,0xF2};
u32  rdr4_wait_answer[MAX_DOORS];
u32  rdr4_receiving[MAX_DOORS];
u32  rdr4_no_response[MAX_DOORS];
bool  rdr4_reader_responded[MAX_DOORS];

u32 rdr4_current_reader, rdr4_reply_byte;

u16 rdr4_q_pace;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

void enable_interrupt_pin(u32 pin);
void disable_interrupt_pin(u32 pin);
void enable_rx_interrupt(u32 rdr4_current_reader);



//=============================================================================================
u8 rdr4_get_char(u16 reader)
  {
  u8 x;
  x = RDR4_rbuf[reader][RDR4_rxo[reader]];
  if (++RDR4_rxo[reader] >= COM1_RX_LEN)
      {
      RDR4_rxo[reader] = 0;
      }
    if (RDR4_rcnt[reader])
      RDR4_rcnt[reader]--;
  return x;
  }

//=============================================================================================
void rdr4_receive_byte(u32 reader)
  {
//  if (rdr4_xxx
  }

//=============================================================================================
void rdr4_send_bit_stream(void)
  {
  if (!rdr4_xmt)
    {
    return;
    }
  if ((rdr4_cmd_curw & 1) == 0)
    {
    switch (rdr4_current_reader)
      {
      case 0: output_high(READER_LED1); break;
      case 1: output_high(READER_LED2); break;
      case 2: output_high(READER_LED3); break;
      case 3: output_high(READER_LED4); break;
      }
    }
  else
    {
    switch (rdr4_current_reader)
      {
      case 0: output_low(READER_LED1); break;
      case 1: output_low(READER_LED2); break;
      case 2: output_low(READER_LED3); break;
      case 3: output_low(READER_LED4); break;
      }
    }
  rdr4_cmd_curw >>= 1;
  if (++rdr4_cmd_bitx > 10)
    {
    if (--rdr4_cmd_len)
      {
      rdr4_cmd_bitx = 0;
      rdr4_cmd_curw =  rdr4_cmd[++rdr4_cmd_idx];
      }
    else
      {
      rdr4_xmt = 0;
      HAL_NVIC_DisableIRQ(TIM2_IRQn);
      enable_rx_interrupt(rdr4_current_reader);
      }
    }
  }

//=============================================================================================
void rdr4_send_command(u32 reader)
  {
  rdr4_current_reader = reader;
  __HAL_TIM_ENABLE(&htim2);
  TIM2->CNT = 0;
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
  __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }

//=============================================================================================
void rdr4_set_state(u32 reader, u8 state)
  {
  u16 chk;
  rdr4_cmd[0] = 0x70A << 1;
  rdr4_cmd[1] = 0x704 << 1;
  rdr4_cmd[2] = 0x701 << 1;
  rdr4_cmd[3] = 0x704 << 1;
  rdr4_cmd[4] = (0x700 | state) << 1;
  chk = -(0x0A + 4 + 1 + 4 + state);
  chk &= 255;
  rdr4_cmd[5] = (0x700 | chk) << 1;
  rdr4_cmd_idx = 0;
  rdr4_cmd_bitx = 0;
  rdr4_cmd_len = 6;
  rdr4_cmd_ch = reader;
  rdr4_cmd_curw = rdr4_cmd[0];
  rdr4_latest_state[reader] = state;
  rdr4_xmt = 1;
  rdr4_send_command(reader);
  }

//=============================================================================================
void rdr4_red(u32 reader)
  {
  rdr4_set_state(reader, 0xF1);
  delay_ms(30);
  rdr4_set_state(reader, 0xF1);
  }
    
//=============================================================================================
void rdr4_green(u32 reader)
  {
  rdr4_set_state(reader, 0xF4);
  }
    
//=============================================================================================
void rdr4_yellow(u32 reader)
  {
  rdr4_set_state(reader, 0xF2);
  }
    
//=============================================================================================
void rdr4_get_status(u32 reader)
  {
//  u16 chk;
  rdr4_cmd[0] = 0x70A << 1;
  rdr4_cmd[1] = 0x703 << 1;
  rdr4_cmd[2] = 0x701 << 1;
  rdr4_cmd[3] = 0x700 << 1;
  rdr4_cmd[4] = 0x7F2 << 1;
  rdr4_cmd_idx = 0;
  rdr4_cmd_bitx = 0;
  rdr4_cmd_len = 5;
  rdr4_cmd_ch = reader;
  rdr4_cmd_curw = rdr4_cmd[0];
  rdr4_xmt = 1;
  rdr4_send_command(reader);
  }

//=============================================================================================
u16 rdr4_rx_count(u32 reader)
  {
  return RDR4_rcnt[reader];
  }
  
//=============================================================================================
void init_rdr4(void)
  {
  __NOP();
  }
 
//=============================================================================================
void rdr4_process_reply(u32 reader)
  {
  u32 key;
  if (rdr4_reply[reader][1] == 9)
    {
    key = make32(rdr4_reply[reader][6],rdr4_reply[reader][7],rdr4_reply[reader][8],rdr4_reply[reader][9]);
    process_key(reader, key, 0);
    }
  }


//=============================================================================================
void handle_rdr4(void)
  {
  static u32 readers_not_respond = 0, active_readers = 0;
  u32 reader;
  u8 chr;
  static bool force_reset = false;
//  rdr4_check_uarts();
  if ((Timer_10mS_Flags & Tmr_10mS_RDR4_TIME))
    {
    Timer_10mS_Flags  &= ~(Tmr_10mS_RDR4_TIME);
    if (++rdr4_q_pace > 4)
      {
      rdr4_q_pace = 0;
      RDR4_rxi[rdr4_query] =  0; 
      RDR4_rxo[rdr4_query] =  0;
      RDR4_rcnt[rdr4_query] = 0;
      if (unlock10[rdr4_query])
        {
        if (unlock10[rdr4_query] < 4)
          {
          rdr4_yellow(rdr4_query);
          }
        else
          {
          rdr4_green(rdr4_query);
          }
        }
      else
        {
        if (++rdr4_wait_answer[rdr4_query] > 5)
          {
          rdr4_wait_answer[rdr4_query] = 0;
          rdr4_no_response[rdr4_query]++;
          readers_not_respond = 0;
          active_readers = 0;
          for (u32 idx = 0; idx < MAX_DOORS; idx++)
            {
            if (rdr4_reader_responded[idx])
              {
              if (rdr4_no_response[idx] > 20)
                {
                force_reset = true;
                }
              active_readers++;
              if (rdr4_no_response[idx])
                {
                readers_not_respond++;
                }
              }
            }
          if (active_readers && (readers_not_respond == active_readers) || force_reset)
            {
            //rdr4_init_system();
            NVIC_SystemReset();
            }
          }  
        rdr4_get_status(rdr4_query);
        }
      if (++rdr4_query >= MAX_DOORS)
        {
        rdr4_query = 0;
        }
      }
    }
  for (reader = 0; reader < MAX_DOORS; reader++)
    {
    while (RDR4_rcnt[reader])
      {
      chr = rdr4_get_char(reader);
      switch (rdr4_rx_state[reader])
        {
        case 0: // wait for preamble
          if (chr == 0x0A)
            {
            rdr4_reply[reader][0] = chr;
            rdr4_rx_state[reader]++;
            }
          break;
        case 1: // store message length
          if (chr > 9)
            {
            rdr4_rx_state[reader]--;
            }
          else
            {
            rdr4_reply[reader][1] = chr;  // data length
            rdr4_rx_state[reader]++;
            rdr4_reply_idx[reader] = 2;
            }
          break;
        case 2: // collect message
          rdr4_reply[reader][rdr4_reply_idx[reader]] = chr;
          if (++rdr4_reply_idx[reader] > rdr4_reply[reader][1] + 2)
            {
            rdr4_process_reply(reader);
            rdr4_rx_state[reader] = 0;
            rdr4_wait_answer[reader] = 0;
            rdr4_no_response[reader] = 0;
            }
          break;
        }
      }
    }
  }

//=============================================================================================
void enable_rx_interrupt(u32 current_reader)
  {
  switch (current_reader)
    {
    case 0: 
      __HAL_GPIO_EXTI_CLEAR_IT(RDR4_EXTI_RX1);
      enable_interrupt_pin(RDR4_EXTI_RX1);
      break;
    case 1: 
      __HAL_GPIO_EXTI_CLEAR_IT(RDR4_EXTI_RX2);
      enable_interrupt_pin(RDR4_EXTI_RX2);
      break;
    case 2: 
      __HAL_GPIO_EXTI_CLEAR_IT(RDR4_EXTI_RX3);
      enable_interrupt_pin(RDR4_EXTI_RX3);
      break;
    case 3: 
      __HAL_GPIO_EXTI_CLEAR_IT(RDR4_EXTI_RX4);
      enable_interrupt_pin(RDR4_EXTI_RX4);
      break;
    }
  }
 
//=============================================================================================
u32 bit_time = 4900;
void rdr4_receive_bit(void)
  {
  u32 pin_state = 0;
  TIM5->ARR = bit_time; // 1 bit
  TIM5->CNT = 0;
  __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);
  
  switch (rdr4_current_reader)
    {
    case 0: 
      pin_state = input(RDR4_PIN_RX1);
      break;
    case 1: 
      pin_state = input(RDR4_PIN_RX2);
      break;
    case 2: 
      pin_state = input(RDR4_PIN_RX3);
      break;
    case 3: 
      pin_state = input(RDR4_PIN_RX4);
      break;
    }
  if (pin_state)
    {
    rdr4_reply_byte |= 1 <<rdr4_reply_bitx;
    }
  if (++rdr4_reply_bitx >= 8)
    {
    __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    rdr4_reply_byte ^= 0xFF;
    RDR4_rbuf[rdr4_current_reader][RDR4_rxi[rdr4_current_reader]] = rdr4_reply_byte & 0xFF;
    rdr4_reply_byte = 0;
    enable_rx_interrupt(rdr4_current_reader);
    if (++RDR4_rxi[rdr4_current_reader] >= COM1_RX_LEN)
      {
      RDR4_rxi[rdr4_current_reader] = 0;
      }
    if (RDR4_rcnt[rdr4_current_reader] < COM1_RX_LEN)
      {
      RDR4_rcnt[rdr4_current_reader]++;
      }
    else
      {
      RDR4_rxo[rdr4_current_reader] = RDR4_rxi[rdr4_current_reader];
      }
    rdr4_receiving[rdr4_current_reader] = 0;
    }
  }

//=============================================================================================
u32 bit_and_half = 6500;
void rdr4_init_char_rx(u32 reader)
  {
  disable_interrupt_pin(RDR4_EXTI_RX1 | RDR4_EXTI_RX2 | RDR4_EXTI_RX3 | RDR4_EXTI_RX4);

  __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);
  __HAL_TIM_ENABLE_IT(&htim5, TIM_IT_UPDATE);
  HAL_NVIC_EnableIRQ(TIM5_IRQn);
  TIM5->ARR = bit_and_half; // 1.5 bits
  TIM5->CNT = 0;
  
  rdr4_receiving[reader] = 1;
  rdr4_reply_byte = 0;
  rdr4_current_reader = reader;
  rdr4_reply_bitx = 0;
  }

