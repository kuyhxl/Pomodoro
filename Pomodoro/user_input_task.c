#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "pomodoro_main.h"
#include "user_input_task.h"
#include "presence_task.h"

// 입력 폴링 간격 (100ms)
#define INPUT_POLL_MS 100U

// 최대 입력 자릿수
#define MAX_INPUT_DIGITS 5

// 대기 중인 숫자 입력
static uint32_t s_pending_value = 0U;
static uint8_t s_pending_digits = 0U;

static void prvShowPending(void)
{
    if (s_pending_digits == 0U)
    {
        printf("[UserInput] Input cleared.\r\n");
    }
    else
    {
        printf("[UserInput] Pending: %u  (press 'm' for minutes, 't' for seconds, 'c' to cancel)\r\n",
               (unsigned)s_pending_value);
    }
}

static void prvAccumulateDigit(int digit)
{
    if (s_pending_digits >= MAX_INPUT_DIGITS)
    {
        printf("[UserInput] Too many digits (max %d). Press 'c' to cancel.\r\n",
               MAX_INPUT_DIGITS);
        return;
    }

    s_pending_value = (s_pending_value * 10U) + (uint32_t)digit;
    s_pending_digits++;
    prvShowPending();
}

static void prvApplyTime(uint32_t seconds)
{
    PomodoroEvent_t event;

    if (seconds == 0U)
    {
        printf("[UserInput] Cannot set zero. Type a number first.\r\n");
        return;
    }

    event.type = EVT_USER_SET_TIME;
    event.payload = seconds;
    xQueueSend(xEventQueue, &event, 0);

    printf("[UserInput] -> SET %u sec  (= %02u:%02u)\r\n",
           (unsigned)seconds,
           (unsigned)(seconds / 60U),
           (unsigned)(seconds % 60U));

    /* Reset accumulator after applying */
    s_pending_value = 0U;
    s_pending_digits = 0U;
}

static void prvSendSimpleEvent(PomodoroEventType_t type, const char *label)
{
    PomodoroEvent_t event;
    event.type = type;
    event.payload = 0U;
    xQueueSend(xEventQueue, &event, 0);
    printf("[UserInput] -> %s\r\n", label);
}

static void prvHandleKey(int key)
{
    // 숫자 입력 처리
    if (key >= '0' && key <= '9')
    {
        prvAccumulateDigit(key - '0');
        return;
    }

    switch (key)
    {
    // 시간 설정
    case 'm':
        prvApplyTime(s_pending_value * 60U);
        break;

    case 't':
        prvApplyTime(s_pending_value);
        break;
    case 'M':
    {
        PomodoroEvent_t event;
        uint32_t secs = s_pending_value * 60U;
        if (secs == 0U)
        {
            printf("[UserInput] Cannot set Zero. Type a number first.\r\n");
            break;
        }
        event.type = EVT_USER_SET_BREAK_TIME;
        event.payload = secs;
        xQueueSend(xEventQueue, &event, 0);
        printf("[UserInput] -> SET BREAK %u sec (=%02u:%02u)\r\n",
               (unsigned)secs,
               (unsigned)(secs / 60U),
               (unsigned)(secs % 60U));
        s_pending_value = 0U;
        s_pending_digits = 0U;
    }
    break;

    case 'T':
    {
        PomodoroEvent_t event;
        uint32_t secs = s_pending_value;
        if (secs == 0U)
        {
            printf("[UserInput] Cannot set zero. Type a number first.\r\n");
            break;
        }
        event.type = EVT_USER_SET_BREAK_TIME;
        event.payload = secs;
        xQueueSend(xEventQueue, &event, 0);
        printf("[UserInput] -> SET BREAK %u sec (=%02u:%02u)\r\n",
               (unsigned)secs,
               (unsigned)(secs / 60U),
               (unsigned)(secs % 60U));
        s_pending_value = 0U;
        s_pending_digits = 0U;
    }
    break;

    case 'c':
    case 'C':
        s_pending_value = 0U;
        s_pending_digits = 0U;
        prvShowPending();
        break;

        /* ---- Session control ---- */
    case 'g':
        prvSendSimpleEvent(EVT_USER_START, "START (Go)");
        break;

    case 'x':
        prvSendSimpleEvent(EVT_USER_STOP, "STOP");
        break;

    case 'a':
        prvSendSimpleEvent(EVT_ALARM_ACK, "ALARM ACK");
        break;

        /* ---- Presence simulation ---- */
    case 'p':
        vPresenceSetSensorAbsent();
        break;

    case 'r':
        vPresenceSetSensorPresent();
        break;

        /* --- Reset / default / counter ---- */
    case 'z':
        prvSendSimpleEvent(EVT_USER_RESET, "RESET (rewind countdown, go IDLE)");
        break;
    case 'd':
        prvSendSimpleEvent(EVT_USER_DEFAULTS, "DEFAULTS (resotre factory times)");
        break;
    case 'k':
        prvSendSimpleEvent(EVT_USER_CLEAR_COUNTER, "Claer Counter");
        break;
        /* ---- Help ---- */
    case 'q':
        prvSendSimpleEvent(EVT_USER_QUIT, "QUIT");
        break;

    case 'h':
        printf("[UserInput] ========== Help ==========\r\n"
               " Set time: type digits, then\r\n"
               " t : study SECONDS m : study MINUTES\r\n"
               " T : break SECONDS M : break MINUTES\r\n"
               " c : cancel pending\r\n"
               " Session:\r\n"
               " g : Go (start) x : Stop\r\n"
               " a : Acknowledge alarm\r\n"
               " Reset:\r\n"
               " z : Reset (rewind, go IDLE)\r\n"
               " d : Defaults (factory times)\r\n"
               " k : Clear pomodoro counter\r\n"
               " Presence (sim):\r\n"
               " p : user absent r : user back\r\n"
               " q : Quit program\r\n"
               " h : this help\r\n"
               "===========================\r\n");
        break;

    default:
        break;
    }
}

void vUserInputTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(INPUT_POLL_MS);
    int key;

    (void)pvParameters;

    printf("[UserInput] Started. Press 'h' for help.\r\n");

    for (;;)
    {
        key = iGetLastKey();
        if (key != 0)
        {
            prvHandleKey(key);
        }
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}