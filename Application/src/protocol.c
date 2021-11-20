///////////////////////////////////////////////////////////////////////////////
// Source file protocol.c
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Communication protocol
///////////////////////////////////////////////////////////////////////////////

#include "protocol.h"
#include "main.h"

//AMA copmilation
#ifdef AMA_COMPILATION
  #define WHO_AM_I DevTypeAMA
#endif

//AMA copmilation
#ifdef RMA_COMPILATION
  #define WHO_AM_I DevTypeRMA
#endif


//RMA copmilation
#if defined (RMA_COMPILATION) || (AMA_COMPILATION)
#include "radio_appli.h"
extern UART_HandleTypeDef huart1;
#endif

//Bootloader Compilation 
#ifdef BOOTLOADER_COMPILATION
extern UART_HandleTypeDef huart1;
#define WHO_AM_I DevTypeBootloader
#endif

//SHELF_MS copmilation
#ifdef SHELF_MS_COMPILATION
extern UART_HandleTypeDef huart1;
#define WHO_AM_I DevTypeSHELF_MS
#endif
static Message msg;


uint32_t short_packets_sent;

/**************************************************
* Function name	: ProtocolSendMessage
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: Builds and sends protocol message
* Notes		: 
**************************************************/
uint32_t zero_length;
ErrorCode ProtocolSendMessage( DevType dst, Opcode opcode, uint8_t* data, uint16_t len )
{
  uint16_t dataLen = len;
  uint16_t msgSize = dataLen + PROTOCOL_HEADER_SIZE;
  
  ErrorCode err = etErrorCodeSuccess;
  //Fill the header
  msg.preamble = MESSAGE_PREAMBLE;
  msg.opcode = opcode;
  msg.len = dataLen;
  msg.dest = dst;
  msg.source = WHO_AM_I;
  
  short_packets_sent++;
  
  //Copy data
  if (dataLen)
    {
    if (dataLen <= MAX_DATA_SIZE)
      {
      memcpy( msg.data, data, dataLen);
      }
    else
      {
      return etErrorCodeWrongSize;
      }
    }
  else if (msg.opcode != OpcodeAck)
    {
    __NOP();
    zero_length++;
    }
  //Put the Checksum
  msg.crc = 0;
  msg.crc = ProtocolCalcCrc((uint8_t*)&msg, msgSize); 

  
//Send message
#if defined (RMA_COMPILATION) || (AMA_COMPILATION)
  switch (dst)
  {
  case DevTypeAMA:
  case DevTypeRMA:  
    //Wait for radio ready
    while(getRadioSM_State() == SM_STATE_WAIT_FOR_TX_DONE ||
          getRadioSM_State() == SM_STATE_SEND_DATA)
    {
      RadioControlHandler();
    }
    radioSendData((uint8_t*)&msg, msgSize);
    
    //Wait for radio ready
    while(getRadioSM_State() == SM_STATE_WAIT_FOR_TX_DONE ||
          getRadioSM_State() == SM_STATE_SEND_DATA)
    {
      RadioControlHandler();
    }
//    short_packets++;
    //Wait for radio ready
    //    while(getRadioSM_State() == SM_STATE_WAIT_FOR_TX_DONE ||
    //          getRadioSM_State() == SM_STATE_SEND_DATA)
//    while(!(getRadioSM_State() == SM_STATE_START_RX))
//    {
//      RadioControlHandler();
//    }
    break;   
  case DevTypePC:
    HAL_UART_Transmit(&huart1, (uint8_t*) &msg, msgSize, msgSize * 1);
    break;  
  default:
    err = etErrorCodeInvDst;
    break;
  }
#endif

  
#ifdef BOOTLOADER_COMPILATION
  switch (dst)
  {
  case DevTypePC:
    HAL_UART_Transmit(&huart1, (uint8_t*) &msg, msgSize, msgSize * 1);
    break;
  default:
    err = etErrorCodeInvDst;
    break;
  }
#endif
  
  return err;
}
/**************************************************
* Function name	: ProtocolCalcCrc
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: Calculates check sum for the protocol messages
* Notes		: //-- CRC-16-CCITT (poly 0x1021) --//
**************************************************/
uint16_t ProtocolCalcCrc(const uint8_t* buff ,uint16_t len)
{
  uint8_t byte;
  uint16_t crc = 0xFFFF;
  
  while (len--)
  {
    byte = crc >> 8 ^ *buff++;
    byte ^= byte>>4;
    crc = (crc << 8) ^ ((uint16_t)(byte << 12)) ^ ((uint16_t)(byte <<5)) ^ ((uint16_t)byte);
  }
  return crc;
}

/**************************************************
* Function name	: ProtocolParseMsg
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: Protocol message collecting state machine
* Notes		: //Preamle -> length -> opcode -> crc -> data...
**************************************************/
static volatile uint32_t startTime;
int ProtocolParseMsg( Message* rxMessage, uint8_t byte)
{
  static int dataIndex = 0;
  
  char rxByte = byte;
  static uint8_t subState = 0;
  volatile static MsgState state = etMsgStateWaitingForPreamble;
  
  
  switch(state)
  {
    //Waiting for maeesage preamble (2 bytes)
  case etMsgStateWaitingForPreamble:
    {
      uint16_t preamble = MESSAGE_PREAMBLE;
      uint8_t  preamble_a = (uint8_t)(preamble & 0xFF);
      uint8_t  preamble_b = (uint8_t)(preamble >> 8 & 0x00FF);
      
      startTime = _GetTime();
      //First byte of preamle
      if(subState == 0)
      {
        if(rxByte == preamble_a)
        {
          rxMessage->preamble = rxByte;
          subState++;
        }
      }
      //Second byte of preamle
      else if(subState == 1)
      {
        if(rxByte == preamble_b)
        {
          rxMessage->preamble |= (rxByte << 8 & 0xFF00);
          state = etMsgStateReadingLength;
          dataIndex = 0;
          subState = 0;
        }else
        {
          subState = 0;
        }
      }
    }  
    break;
    
    //Reading length (2 bytes)
  case etMsgStateReadingLength:
    {
      //First byte
      if(subState == 0)
      {  
        subState++;
        rxMessage->len = rxByte;
      }
      //Second byte
      else if(subState == 1)
      {
        subState = 0;
        rxMessage->len |= (rxByte << 8 & 0xFF00);
        //Valid length?
        if(rxMessage->len <= MAX_DATA_SIZE ) 
        {
          state = etMsgStateReadingOpcode; //Next state
        }
        else
        {
          state = etMsgStateWaitingForPreamble; //Invalid data size
        }
      }
    }
    break;
    
    //Reading opcode
  case etMsgStateReadingOpcode:
    rxMessage->opcode = (Opcode)rxByte;
    state = etMsgStateReadingSource; //Next state
    break;
    
    //Reading source
  case etMsgStateReadingSource :
    rxMessage->source = (DevType)rxByte;
    state = etMsgStateReadingDest; //Next state        
    break;
    
  case etMsgStateReadingDest:
    //Reading destination
    state = etMsgStateReadingCrc; //Next state
    rxMessage->dest = (DevType)rxByte;
    break;
    
    //Reading CRC (2 bytes)
  case etMsgStateReadingCrc:
    //First byte
    if(subState == 0)
      {  
        rxMessage->crc = rxByte;
        subState++;
      }
      //Second byte
      else if(subState == 1)
      {
        rxMessage->crc |= (rxByte << 8);
        if(rxMessage->len == 0)
        {
          state = etMsgStateMessageReady;//Next state
        }
        else 
        {
          state = eMsgStatetReadingData; //Next state
        }
        subState = 0;
      }
    break;
    
    //Collecting data
  case eMsgStatetReadingData:
      //Overflow protection
      if(dataIndex <= MAX_DATA_SIZE)
      {
        //Collect the data
        rxMessage->data[dataIndex] = rxByte;
        dataIndex++;
      }
      //End of the data 
      if(dataIndex >= rxMessage->len)
      {
        dataIndex = 0;
        state = etMsgStateMessageReady;
      }
    break;
  }
  
  //Timeout 
  if( startTime + USART_PROTOCOL_TIMEOUT < _GetTime())
  {
    //ProtocolSendMessage( OpcodeMessageTimeout, NULL,0);  //send Nack message  
    state = etMsgStateWaitingForPreamble;
  }
  
  if( state == etMsgStateMessageReady )
  {
    state = etMsgStateWaitingForPreamble;
    return etMsgStateMessageReady;
  }
  
  return state;
}



/**************************************************
* Function name	: ProtocolMessageValidateCrc
* Returns	:	
* Arg		: 
* Created by	: Lev Zoosmanovskiy
* Date created	: 20/01/2020
* Description	: Validates the checksum
* Notes		: 
**************************************************/
uint8_t ProtocolMessageValidateCrc( Message* pMsg, uint16_t val)
{  
  pMsg->crc = 0;
  pMsg->crc = ProtocolCalcCrc((const uint8_t*) pMsg ,pMsg->len + PROTOCOL_HEADER_SIZE);
  return ( val == pMsg->crc );
}

