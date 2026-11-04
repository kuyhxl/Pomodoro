#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define DISPLAY_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

void vDisplayTask(void* pvParameters);

#endif /* DISPLAY_TASK_H */