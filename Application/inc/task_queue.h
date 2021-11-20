///////////////////////////////////////////////////////////////////////////////
// Source file task_queue.h
//
// Author:  Zoosmanovskiy Lev.
// email:   lev@ztech.co.il
// Copyright 2015, Zoosmanovskiy Lev.
// Simple task queue system
///////////////////////////////////////////////////////////////////////////////
#ifndef __TASK_QUEUE_H
#define __TASK_QUEUE_H
#include "stdint.h"

#define MAX_LISTENERS 10

#define TASK_RUN_ONE_SHOT 	0
#define TASK_RUN_REPEATEDLY 1

typedef void (*task_cb)(void *opaq);

typedef int TaskHandle;

void init_tasks(void);

TaskHandle add_task(task_cb cb);

uint8_t exec_task(TaskHandle taskHandle, uint32_t period, uint8_t repeat, void *opaq);

void cancel_task(TaskHandle taskHandle);

void event_loop(void);

#endif
