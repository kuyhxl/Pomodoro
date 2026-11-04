#ifndef PRESENCE_TASK_H
#define PRESENCE_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define PRESENCE_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define PRESENCE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

void vPresenceDetectionTask(void* pvParameters);

void vPresenceSetSensorAbsent(void);
void vPresenceSetSensorPresent(void);

#endif /* PRESENCE_TASK_H */