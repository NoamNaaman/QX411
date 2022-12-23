/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "setup.h"

#define EE_ADDR 0xAA

//#define  SDA    GPIOB,9
//#define  SCL    GPIOB,6
//#define  WP     GPIOB,7

#define  IIC_READ 1

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_Delay(void)
  {
  delay_us(2);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void  init_i2c(void)
  {
  output_drive(SCL);
  output_drive(SDA);
  output_high(SCL);
  output_high(SDA);
//  output_drive(WP);
//  output_high(WP);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_SendAck(void)
  {
  output_low(SDA);
  IIC_Delay();
  output_high(SCL);
  IIC_Delay();
  output_low(SCL);
  IIC_Delay();
  output_high(SDA);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_SendOne(void)
  {
  output_high(SDA);
  IIC_Delay();
  output_high(SCL);
  IIC_Delay();
  output_low(SCL);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------
// Sends 8 bits to I2C device
// Gets the byte to send from W register
//--------------------------------------------------------------------------
u32 IIC_WriteByte(u8  send_bits)
  {
  u16 count;
  u32   Ack;
  output_drive(SCL);
  output_drive(SDA);
  for (count = 0; count < 8; count++)
    {
    if (send_bits & 0x80)
      {
      output_high(SDA);
      }
    else
      {
      output_low(SDA);
      }
    send_bits <<= 1;
    
    IIC_Delay();
    output_high(SCL);
    IIC_Delay();
    output_low(SCL);
    }
  output_high(SDA);

  input_pullup(SDA);
  IIC_Delay();
  output_high(SCL);
  IIC_Delay();
  Ack = input(SDA);
  output_low(SCL);
  input_pullup(SDA);
  delay_us(1);
  return !Ack;
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------
// Receives 8 bits from I2C device
// returns the received byte in the W reg.
//--------------------------------------------------------------------------
u8 IIC_ReadByte(u8 ack)
  {
  u8 rec_bits = 0, rbx = 0, rbz;
  u16 count;
  output_drive(SCL);
//  output_low(SDA);
  input_pullup(SDA);
  rbx = input(SDA);
  IIC_Delay();
  for (count = 0; count < 8; count++)
    {
    rec_bits <<= 1;
    rbx <<= 1;
    output_high(SCL);
    IIC_Delay();
    if (input(SDA))
      {
      rec_bits |= 1;
      }
    output_low(SCL);
    if (input(SDA))
      {
      rbx |= 1;
      }
    IIC_Delay();
    }
  output_drive(SDA);
  output_high(SDA);
  if (ack)
    {
    IIC_SendAck();
    }
  delay_us(5);
  if (rbx || rec_bits)
    {
    rbz = 0;
    }
  return rec_bits;
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------
// Sends start unsigned to I2C device
//--------------------------------------------------------------------------
void IIC_Start(void)
  {
  output_high(SDA);
  IIC_Delay();
  IIC_Delay();
  output_high(SCL);
  IIC_Delay();
  IIC_Delay();
  output_low(SDA);
  IIC_Delay();
  IIC_Delay();
  output_low(SCL);
  IIC_Delay();
  IIC_Delay();
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------
// Sends stop unsigned to I2C device
//--------------------------------------------------------------------------
void IIC_Stop(void)
  {
  output_drive(SDA);
  output_low(SDA);
  IIC_Delay();
  IIC_Delay();
  output_high(SCL);
  IIC_Delay();
  IIC_Delay();
  output_high(SDA);
  IIC_Delay();
  }


/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_PacketPreamble(u16 Address)
  {
//  IIC_Polling();
  IIC_Start();
  IIC_WriteByte(EE_ADDR);
  IIC_WriteByte(Address >> 8);
  IIC_WriteByte(Address & 255);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_PrepareToRead(u16 Address)
  {
  IIC_PacketPreamble(Address);
  IIC_Stop();
  IIC_Start();
  IIC_WriteByte(EE_ADDR | IIC_READ);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_CloseRead(void)
  {
  IIC_ReadByte(0);
  IIC_SendOne();
  IIC_Stop();
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_ReadBytes(u16 Address, u8 *bufptr, u16 n_Len)
  {
  IIC_PrepareToRead(Address);
  while (n_Len--)
    {
    *bufptr++ = IIC_ReadByte(0);
    if (n_Len)
      {
      IIC_SendAck();
      }
    }
  IIC_CloseRead();
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void IIC_WriteBytes(u16 Address,u8 *bufptr,  u16 n_Len)
  {
//  output_low(WP);
  IIC_PacketPreamble(Address);
  while (1)
    {
    IIC_WriteByte(*bufptr++);
    ++Address;
    if (!--n_Len)
      {
      break;
      }
    }
  IIC_Stop();
  delay_ms(5);
//  output_high(WP);
  }

/////////////////////////////////////////////////////////////////////
// Name:        
// Description: 
// Parameters:  
// Returns:     NONE
/////////////////////////////////////////////////////////////////////
void I2C_write(u8 dev_addr, u16 Address, u32 addr_size, u8 *data, u32 data_len)
  {
  IIC_Start();
  IIC_WriteByte(dev_addr);
  if (addr_size == 2)
    {
    IIC_WriteByte(Address >> 8);
    }
  IIC_WriteByte(Address & 255);
  while (1)
    {
    IIC_WriteByte(*data++);
    if (!--data_len)
      {
      break;
      }
    }
  IIC_Stop();
  delay_ms(5);
  }
