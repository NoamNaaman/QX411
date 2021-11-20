///////////////////////////////////////////////////////////////////////////////
// Source file protocol.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Communication protocol
///////////////////////////////////////////////////////////////////////////////
#ifndef __PROTOCOL_H_
#define __PROTOCOL_H_

#include "string.h"
#include "main.h"


#define MESSAGE_PREAMBLE	0xF8AB
#define PROTOCOL_UART_SPEED     115200

#define PROTOCOL_HEADER_SIZE	9
#define PROTOCOL_CRC_SIZE       2
#define MAX_DATA_SIZE           280
#define USART_PROTOCOL_TIMEOUT  1000        //milli Seconds

#define _GetTime        HAL_GetTick


//Protocol operation codes
typedef enum {
            //Service
            OpcodeZero             = 0,
            OpcodeGetInfo          = 1,
            OpcodeGetSelfTest      = 2,
            //Firmware
            OpcodeStartFirmware    = 3,
            OpcodeProcessFirmware  = 4,
            OpcodeFinishFirmware   = 5,
            OpcodeEraseFirmware    = 6,
            OpcodePrepareFirmware  = 7,
            //Big packets
            OpcodeStartBigPacket   = 8,
            OpcodeProcessBigPacket = 9,
            OpcodeEndBigPacket     = 10,
            //Params
            OpcodeSetParam         = 11,
            OpcodeGetParam         = 12,
            //Protocol
            OpcodeMessageTimeout   = 13,
            OpcodeAck              = 14,
            OpcodeNack             = 15,
            
            OpcodeLastOpcode
}Opcode;

//Protocol sub operation codes
typedef enum {
  ParamOpcodeSetTime,
  ParamOpcodeSetYob,
  ParamOpcodeLastOpcode
}ParamOpcode;

//Protocol destination enums
typedef enum {
  DevTypePC = 0,
  DevTypeRMA,
  DevTypeAMA,
  DevTypeSHELF_MS,
  DevTypeBootloader,
  DevTypeLast
}DevType;

//Protocol message
typedef struct __attribute__ ((packed)) __Message
{
  uint16_t  preamble;
  uint16_t  len;
  Opcode    opcode;
  DevType   source;
  DevType   dest;
  uint16_t  crc;
  uint8_t   data[MAX_DATA_SIZE];
}Message;



//Posible error codes
typedef enum 
{
  etErrorCodeSuccess = 0,
  etErrorCodeUsartNoData,
  etErrorCodeHwError,
  etErrorCodeDefineAnyError,
  etErrorCodeCrcError,
  etErrorCodeInvDst,
  etErrorCodeWrongSize
}ErrorCode; 	

//Protocol states
typedef enum
{
  etMsgStateWaitingForPreamble = 0,
  etMsgStateReadingLength,
  etMsgStateReadingSource,
  etMsgStateReadingDest,
  etMsgStateReadingOpcode,
  etMsgStateReadingCrc,
  eMsgStatetReadingData,
  etMsgStateMessageReady,
  etMsgTimeout
}MsgState;


//Prototypes
ErrorCode ProtocolSendMessage(DevType dst, Opcode opcode, uint8_t* data, uint16_t len );
ErrorCode ProtocolSendBuffer( uint8_t* buffer, uint8_t length );
ErrorCode ProtocolRecieveBuffer( uint8_t* buffer, uint8_t length );
int ProtocolParseMsg( Message* rxMessage , uint8_t byte);
uint8_t ProtocolMessageValidateCrc( Message* Pmsg, uint16_t val);
uint16_t ProtocolCalcCrc(const uint8_t* buff ,uint16_t len);

#endif // __PROTOCOL_H_ 
