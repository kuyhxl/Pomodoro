#ifndef TIMER_TASK_H
#define TIMER_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define TIMER_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define TIMER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

void vTimerTask(void* pvParameters);


#endif /* TIMER_TASK_H */