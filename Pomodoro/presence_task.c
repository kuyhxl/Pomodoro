#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "presence_task.h"

#define SAMPLE_PERIOD_MS 1000U
#define CONFIRM_SAMPLES_REQUIRED 3U

// 센서가 현재 읽고 있는 값으로 기본값: PRESENT
static volatile BaseType_t s_sensor_absent = pdFALSE;

// SessionMgr에 마지막으로 보고된 상태로 이 값이 변경될 때만 이벤트를 큐에 넣는다.
static BaseType_t s_last_reported_absent = pdFALSE;

// 현재 센서 값이 s_last_reported_absent와 다르게 연속으로 몇 초 동안 유지되었는지 나타냄
static uint8_t s_confirm_count = 0U;

void vPresenceSetSensorAbsent(void)
{
    if (s_sensor_absent == pdFALSE)
    {
        s_sensor_absent = pdTRUE;
        s_confirm_count = 0U; // 새로운 판독 시작
        printf("[PresenceTask] Sensor input: ABSENT (will confirm over 3 seconds)\r\n");
    }
    else
    {
        printf("[PresenceTask] Sensor input: ABSENT (already)\r\n");
    }
}

void vPresenceSetSensorPresent(void)
{
    if (s_sensor_absent == pdTRUE)
    {
        s_sensor_absent = pdFALSE;
        s_confirm_count = 0U;
        printf("[PresenceTask] Sensor input: PRESENT (will confirm over 3 seconds)\r\n");
    }
    else
    {
        printf("[PresenceTask] Sensor input: PRESENT (already)\r\n");
    }
}

void vPresenceDetectionTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);

    PomodoroEvent_t event;

    (void)pvParameters;

    printf("[PresenceTask] Started. p = sensor sees absence, r = sensor sees presence.\r\n"
           "[PresenceTask] Action triggers after 3 consecutive seconds.\r\n");

    for (;;)
    {
        BaseType_t reading = s_sensor_absent;

        if (reading != s_last_reported_absent)
        {
            // 센서 판독값이 마지막으로 보고된 상태와 다르면 연속된 샘플 개수를 센다
            s_confirm_count++;
            printf("[PresenceTask] Sensor reports %s (%u/%u)\r\n",
                   reading == pdTRUE ? "ABSENT" : "PRESENT",
                   (unsigned)s_confirm_count,
                   (unsigned)CONFIRM_SAMPLES_REQUIRED);

            if (s_confirm_count >= CONFIRM_SAMPLES_REQUIRED)
            {
                // 확인됨. 이벤트를 전송하고 마지막으로 보고된 상태를 업데이트한
                if (reading == pdTRUE)
                {
                    event.type = EVT_PRESENCE_ABSENT;
                    event.payload = 0U;
                    xQueueSend(xEventQueue, &event, 0);
                    printf("[PresenceTask] Confirmed ABSENT. Notifying SessionMgr.\r\n");
                }
                else
                {
                    event.type = EVT_PRESENCE_PRESENT;
                    event.payload = 0U;
                    xQueueSend(xEventQueue, &event, 0);
                    printf("[PresenceTask] Confirmed PRESENT. Notifying SessionMgr.\r\n");
                }

                s_last_reported_absent = reading;
                s_confirm_count = 0U;
            }
        }
        else
        {
            // 판독값이 마지막으로 보고된 상태와 일치하면 카운터를 초기화a
            s_confirm_count = 0U;
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}