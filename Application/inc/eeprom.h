///////////////////////////////////////////////////////////////////////////////
// Source file eeprom.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// 24CM01 eeprom read/write driver
///////////////////////////////////////////////////////////////////////////////

#define _24CM01_ADDRESS 0x55<<1

#define EEPROM_WP_PIN GPIOB, GPIO_PORT_8

#ifndef __EEPROM_24CM01_H_
#define __EEPROM_24CM01_H_

#include "main.h"


void eepromRead(uint16_t addr, uint8_t* dst, uint16_t len);
void eepromWrite(uint16_t addr, uint8_t* data, uint16_t len);
#endif //__EEPROM_24M05_H_