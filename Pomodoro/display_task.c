#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "display_task.h"

#define DISPLAY_PERIOD_MS 1000U

void vDisplayTask(void* pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    PomodoroState_t state;
    uint32_t study_remaining, study_acc;
    uint32_t break_remaining, break_acc;
    uint32_t completed;

    (void)pvParameters;

    xLastWakeTime = xTaskGetTickCount();

    for (;; ) {
        // 세션 데이터의 일관된 스냅샷을 가져온다
        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        state = xSession.state;
        study_remaining = xSession.study_seconds_remaining;
        study_acc = xSession.study_seconds_accumulated;
        break_remaining = xSession.break_seconds_remaining;
        break_acc = xSession.break_seconds_accumulated;
        completed = xSession.completed_pomodoros;
        xSemaphoreGive(xStateMutex);

        printf("[DisplayTask] state=%-8s | Study=%02u:%02u (acc %02u:%02u) | Break=%02u:%02u (acc %02u:%02u)| Done=%u\r\n",
                pcPomodoroStateName(state),
                (unsigned) (study_remaining / 60U),
                (unsigned) (study_remaining % 60U),
                (unsigned) (study_acc / 60U),
                (unsigned) (study_acc % 60U),
                (unsigned) (break_remaining / 60U),
                (unsigned) (break_remaining % 60U),
                (unsigned) (break_acc / 60U),
                (unsigned) (break_acc % 60U),
                (unsigned) completed);
        
        // 정확히 1000ms 주기로 실행
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }

}