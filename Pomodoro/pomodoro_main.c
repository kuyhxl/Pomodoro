#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_main.h"
#include "pomodoro_api.h"

#include "display_task.h"
#include "session_manager.h"
#include "timer_task.h"
#include "user_input_task.h"
#include "presence_task.h"
#include "logging_task.h"

// 디버깅/추적용 더미 변수
void* RecorderDataPtr = NULL;

// 전역 RTOS 객체 정의
QueueHandle_t xEventQueue = NULL;
SemaphoreHandle_t xStateMutex = NULL;
SemaphoreHandle_t xAlarmSemaphore = NULL;
PomodoroSession_t xSession = {0};

// 초기 설정값
#define EVENT_QUEUE_LENGHTH 16
#define DEFAULT_STUDY_SECONDS POMO_DEFAULT_STUDY_SECONDS
#define DEFAULT_BREAK_SECONDS POMO_DEFAULT_BREAK_SECONDS

void vStartPomodoroSystem(void) {
    printf("[BOOT] Pomodoro system starting...\r\n");

    // 1. Create IPC objects
    xEventQueue = xQueueCreate(EVENT_QUEUE_LENGHTH, sizeof(PomodoroEvent_t));
    xStateMutex = xSemaphoreCreateMutex();
    xAlarmSemaphore = xSemaphoreCreateBinary();

    configASSERT(xEventQueue != NULL);
    configASSERT(xStateMutex != NULL);
    configASSERT(xAlarmSemaphore != NULL);

    // 2. Initialize session state
    xSession.state = POMO_STATE_IDLE;
    xSession.study_seconds_total = DEFAULT_STUDY_SECONDS;
    xSession.study_seconds_remaining = DEFAULT_STUDY_SECONDS;
    xSession.study_seconds_accumulated = 0U;
    xSession.break_seconds_total = DEFAULT_BREAK_SECONDS;
    xSession.break_seconds_remaining = DEFAULT_BREAK_SECONDS;
    xSession.break_seconds_accumulated = 0U;
    xSession.completed_pomodoros = 0U;
    xSession.last_completed = LAST_COMPLETED_NONE;

    // 3. Create tasks
    xTaskCreate(
                vSessionManagerTask,
                "SessMgr",
                SESSION_MGR_STACK_SIZE,
                NULL,
                SESSION_MGR_PRIORITY,
                NULL
                );
    
    xTaskCreate(
        vTimerTask,
        "Timer",
        TIMER_TASK_STACK_SIZE,
        NULL,
        TIMER_TASK_PRIORITY,
        NULL
    );

    xTaskCreate(
        vUserInputTask,
        "UserIn",
        USER_INPUT_TASK_STACK_SIZE,
        NULL,
        USER_INPUT_TASK_PRIORITY,
        NULL
    );

    xTaskCreate(
        vDisplayTask,
        "Display",
        DISPLAY_TASK_STACK_SIZE,
        NULL,
        DISPLAY_TASK_PRIORITY,
        NULL
    );

    xTaskCreate(vPresenceDetectionTask, "Presence",
                PRESENCE_TASK_STACK_SIZE, NULL,
                PRESENCE_TASK_PRIORITY, NULL);
    xTaskCreate(vLoggingSyncTask, "Logging",
                LOGGING_TASK_STACK_SIZE, NULL,
                LOGGING_TASK_PRIORITY, NULL);

    printf("[BOOT] IPC objects created. Starting scheduler...\r\n");
    printf("[BOOT] Tip: press 'h' for keyboard help. \r\n");

    // 4. vTaskStartScheduler();
    vTaskStartScheduler();

    for (;; );
}
