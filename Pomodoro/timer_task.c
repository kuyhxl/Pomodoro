#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "timer_task.h"

#define TIMER_PERIOD_MS 1000U

void vTimerTask(void* pvParameters) {

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TIMER_PERIOD_MS);

    PomodoroEvent_t event;
    PomodoroState_t state_now;
    BaseType_t bExpired;

    (void)pvParameters;

    for (;; ) {
        bExpired = pdFALSE;

        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        state_now = xSession.state;

        if (state_now == POMO_STATE_STUDYING) {
            if (xSession.study_seconds_remaining > 0U) {
                xSession.study_seconds_remaining--;
                xSession.study_seconds_accumulated++;
                if (xSession.study_seconds_remaining == 0U) {
                    bExpired = pdTRUE;
                }
            }
        
        }
        else if (state_now == POMO_STATE_BREAK) {
            if (xSession.break_seconds_remaining > 0U) {
                xSession.break_seconds_remaining--;
                xSession.break_seconds_accumulated++;
                if (xSession.break_seconds_remaining == 0U) {
                    bExpired = pdTRUE;
                }
            }
        }
        xSemaphoreGive(xStateMutex);

        if (bExpired == pdTRUE) {
            event.type = EVT_TIMER_EXPIRED;
            event.payload = 0U;
            xQueueSend(xEventQueue, &event, 0);
            printf("[TimerTask] Timer expired (state was %s)\r\n", pcPomodoroStateName(state_now));
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}