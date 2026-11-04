#ifndef LOGGING_TASK_H
#define LOGGING_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define LOGGING_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define LOGGING_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE *4)

void vLoggingSyncTask(void* pvParameters);

#endif /* LOGGING_TASK_H */