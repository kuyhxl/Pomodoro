#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "FreeRTOS.h"
#include "task.h"

#define SESSION_MGR_PRIORITY (tskIDLE_PRIORITY + 5)
#define SESSION_MGR_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

void vSessionManagerTask(void* pvParameters);

#endif /* SESSION_MANAGER_H */