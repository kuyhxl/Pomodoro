#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "session_manager.h"
#include "pomodoro_main.h"


void vSessionManagerTask(void* pvParameters)
{
    PomodoroEvent_t event;
    PomodoroState_t state_now;

    (void)pvParameters;

    for (;; )
    {
        // 이벤트 큐에서 이벤트가 들어올 때까지 대기
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        state_now = xPomodoroGetState();

        switch (event.type)
        {
        // ===== 사용자 명령 ===== 
        case EVT_USER_START:
            if (state_now == POMO_STATE_IDLE || state_now == POMO_STATE_SETTING)
            {
                // IDLE -> SETTING -> STUDYING 
                if (state_now == POMO_STATE_IDLE) {
                    xPomodoroRequestTransition(POMO_STATE_SETTING, "SessionMgr");
                }
                
                // 남은 시간을 설정된 전체 시간으로 초기화
                xSemaphoreTake(xStateMutex, portMAX_DELAY);
                xSession.study_seconds_remaining = xSession.study_seconds_total;
                xSession.break_seconds_remaining = xSession.break_seconds_total;
                xSemaphoreGive(xStateMutex);

                xPomodoroRequestTransition(POMO_STATE_STUDYING, "SessionMgr");
            }
            else if (state_now == POMO_STATE_PAUSED)
            {
                // 수동 재개
                xPomodoroRequestTransition(POMO_STATE_STUDYING, "SessionMgr");
            }
            break;

        case EVT_USER_STOP:
            // Stop -> back to IDLE from any active state
            if (state_now == POMO_STATE_STUDYING ||
                state_now == POMO_STATE_PAUSED ||
                state_now == POMO_STATE_BREAK ||
                state_now == POMO_STATE_SETTING)
            {
                if (state_now == POMO_STATE_SETTING)
                {
                    xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
                }
                else
                {
                    xPomodoroRequestTransition(POMO_STATE_ALARM, "SessionMgr");
                    xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
                }
            }
            break;

        case EVT_USER_SET_TIME:
            // Study time Accepted in any state
        {
            uint32_t new_secs = event.payload;
            if (new_secs == 0U) {
                printf("[SessionMgr] SET_TIME ignored (zero).\r\n");
                break;
            }
            xSemaphoreTake(xStateMutex, portMAX_DELAY);
            xSession.study_seconds_total = new_secs;

            if (state_now == POMO_STATE_STUDYING || state_now == POMO_STATE_PAUSED) {
                xSession.study_seconds_remaining = new_secs;
            }
            else if (state_now == POMO_STATE_IDLE || state_now == POMO_STATE_SETTING) {
                xSession.study_seconds_remaining = new_secs;
            }
            xSemaphoreGive(xStateMutex);

            printf("[SessionMgr] Study time set: %u sec\r\n", (unsigned)new_secs);
            
        }
        break;
        
        case EVT_ALARM_ACK:
            if (state_now == POMO_STATE_ALARM)
            {
                LastCompleted_t last;
                
                xSemaphoreTake(xStateMutex, portMAX_DELAY);
                last = xSession.last_completed;
                xSemaphoreGive(xStateMutex);
                
                if (last == LAST_COMPLETED_STUDY) {
                    // Study just ended -> start the break
                    xSemaphoreTake(xStateMutex, portMAX_DELAY);
                    xSession.break_seconds_remaining = xSession.break_seconds_total;
                    xSemaphoreGive(xStateMutex);

                    xPomodoroRequestTransition(POMO_STATE_BREAK, "SessionMgr");
                }
                else if (last == LAST_COMPLETED_BREAK) {
                    // Break just ended -> back to IDLE
                    xSemaphoreTake(xStateMutex, portMAX_DELAY);
                    xSession.study_seconds_remaining = xSession.study_seconds_total;
                    xSession.break_seconds_remaining = xSession.break_seconds_total;
                    xSession.last_completed = LAST_COMPLETED_NONE;
                    xSemaphoreGive(xStateMutex);

                    xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
                }
                else {
                    // 알람 중 수동으로 중지됨 (바로 IDLE로 복귀)
                    xSemaphoreTake(xStateMutex, portMAX_DELAY);
                    xSession.study_seconds_remaining = xSession.study_seconds_total;
                    xSession.break_seconds_remaining = xSession.break_seconds_total;
                    xSemaphoreGive(xStateMutex);

                    xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
                }
            }
            break;

            /* ===== Sensor events ===== */
        case EVT_PRESENCE_ABSENT:
            if (state_now == POMO_STATE_STUDYING)
            {
                xPomodoroRequestTransition(POMO_STATE_PAUSED, "PresenceTask");
            }
            break;

        case EVT_PRESENCE_PRESENT:
            if (state_now == POMO_STATE_PAUSED)
            {
                xPomodoroRequestTransition(POMO_STATE_STUDYING, "PresenceTask");
            }
            break;

            /* ===== Timer expiration ===== */
        case EVT_TIMER_EXPIRED:
            if (state_now == POMO_STATE_STUDYING || state_now == POMO_STATE_PAUSED)
            {
                // 공부 시간 종료 
                xPomodoroRequestTransition(POMO_STATE_ALARM, "TimerTask");

                xSemaphoreTake(xStateMutex, portMAX_DELAY);
                xSession.completed_pomodoros++;
                xSession.last_completed = LAST_COMPLETED_STUDY;
                xSemaphoreGive(xStateMutex);

                // LoggingTask 깨우기 
                xSemaphoreGive(xAlarmSemaphore);
            }
            else if (state_now == POMO_STATE_BREAK)
            {
                xPomodoroRequestTransition(POMO_STATE_ALARM, "TimerTask");
                
                xSemaphoreTake(xStateMutex, portMAX_DELAY);
                xSession.last_completed = LAST_COMPLETED_BREAK;
                xSemaphoreGive(xStateMutex);

                xSemaphoreGive(xAlarmSemaphore);
            }
            break;
        case EVT_USER_RESET:
        {
            if (state_now != POMO_STATE_IDLE) {
                xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
            }

            xSemaphoreTake(xStateMutex,portMAX_DELAY);
            xSession.study_seconds_remaining = xSession.study_seconds_total;
            xSession.break_seconds_remaining = xSession.break_seconds_total;
            xSession.last_completed = LAST_COMPLETED_NONE;
            xSemaphoreGive(xStateMutex);

            printf("[SessionMgr] RESET: state -> IDLE, countdown rewound.\r\n");
        }
            break;

        case EVT_USER_DEFAULTS:
            // d: 시간을 기본값으로 초기화
        {
            if (state_now != POMO_STATE_IDLE) {
                xPomodoroRequestTransition(POMO_STATE_IDLE, "SessionMgr");
            }

            xSemaphoreTake(xStateMutex, portMAX_DELAY);
            xSession.study_seconds_remaining = POMO_DEFAULT_STUDY_SECONDS;
            xSession.break_seconds_remaining = POMO_DEFAULT_BREAK_SECONDS;
            xSession.study_seconds_total = POMO_DEFAULT_STUDY_SECONDS;
            xSession.break_seconds_total = POMO_DEFAULT_BREAK_SECONDS;
            xSession.last_completed = LAST_COMPLETED_NONE;
            xSemaphoreGive(xStateMutex);

            printf("[SessionMgr] DEFAULTS: study=%u sec, break=%u sec.\r\n",
                (unsigned)POMO_DEFAULT_STUDY_SECONDS,
                (unsigned)POMO_DEFAULT_BREAK_SECONDS);
        }
            break;

        case EVT_USER_CLEAR_COUNTER:
            // k: 완료된 뽀모도로 수 0으로 초기화
            xSemaphoreTake(xStateMutex, portMAX_DELAY);
            xSession.completed_pomodoros = 0U;
            xSession.study_seconds_accumulated = 0U;
            xSession.break_seconds_accumulated = 0U;
            xSemaphoreGive(xStateMutex);
            printf("[SessionMgr] Counter Cleared. \r\n");
            break;

        case EVT_USER_SET_BREAK_TIME:
            // 휴식 시간. set_time과 동일한 정책이지만 휴식 단계에 적용
        {
            uint32_t new_secs = event.payload;
            if (new_secs == 0U ) {
                printf("[SessionMgr] SET_BREAK_TIME ignored (zero). \r\n");
                break;
            }

            xSemaphoreTake(xStateMutex, portMAX_DELAY);
            xSession.break_seconds_total = new_secs;

            if (state_now == POMO_STATE_BREAK) {
                xSession.break_seconds_remaining = new_secs;
            }
            else if (state_now == POMO_STATE_IDLE || state_now == POMO_STATE_SETTING) {
                xSession.break_seconds_remaining = new_secs;
            }
            xSemaphoreGive(xStateMutex);
            if (state_now == POMO_STATE_IDLE) {
                xPomodoroRequestTransition(POMO_STATE_SETTING, "SessionMgr");
            }
            printf("[SessionMgr] Break time set: %u sec\r\n", (unsigned) new_secs);
        }
        break;

        case EVT_USER_QUIT:
            printf("[SessionMgr] Quit requested. Goodbye! \r\n");
            fflush(stdout);
            exit(0);
            break;

        default:
            printf("[SessionMgr] Unknown event type: %d\r\n", (int)event.type);
            break;
        }
    }
}