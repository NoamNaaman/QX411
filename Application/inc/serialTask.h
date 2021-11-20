///////////////////////////////////////////////////////////////////////////////
// Source file serial_comm.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2020, Zoosmanovskiy Lev.
// Serial communication 
///////////////////////////////////////////////////////////////////////////////


#ifndef __SERIAL_COMM_H_
#define __SERIAL_COMM_H_

#include "main.h"
#include "circ_buff.h"

#define USART_RX_BUFFER_SIZE 256
#define SERIAL_TASK_PERIOD 1

void initSerialTask(void);
tCircularBuffer* getHostCircBuffPtr(void);
#endif