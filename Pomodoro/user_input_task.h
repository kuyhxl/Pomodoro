#ifndef USER_INPUT_TASK_H
#define USER_INPUT_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define USER_INPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define USER_INPUT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

void vUserInputTask(void* pvParameters);

#endif /* USER_INPUT_TASK_H */