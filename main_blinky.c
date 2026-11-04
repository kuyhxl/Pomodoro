#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define mainDISPLAY_TASK_PRIORITY          ( tskIDLE_PRIORITY + 1 )
#define mainPRESENCE_TASK_PRIORITY         ( tskIDLE_PRIORITY + 2 )
#define mainTIMER_TASK_PRIORITY            ( tskIDLE_PRIORITY + 2 )

/* Task periods. */
#define mainDISPLAY_FREQUENCY_MS           pdMS_TO_TICKS( 1000UL )
#define mainABSENT_AFTER_MS                pdMS_TO_TICKS( 3000UL )
#define mainRESUME_AFTER_MS                pdMS_TO_TICKS( 3000UL )
#define mainALARM_AFTER_MS                 pdMS_TO_TICKS( 10000UL )

/*-----------------------------------------------------------*/

static void vDisplayTask(void* pvParameters);
static void vPresenceTask(void* pvParameters);
static void vTimerTask(void* pvParameters);
static const char* pcStateToString(int state);
static BaseType_t xPomodoroRequestTransition(int newState, const char* caller);

/*-----------------------------------------------------------*/

typedef enum
{
    STATE_IDLE = 0,
    STATE_STUDYING,
    STATE_PAUSED,
    STATE_ALARM
} PomodoroState_t;

static SemaphoreHandle_t xStateMutex = NULL;
static PomodoroState_t gCurrentState = STATE_STUDYING;

/*-----------------------------------------------------------*/

void main_blinky(void)
{
    printf("\r\nStarting Pomodoro prototype demo.\r\n\r\n");

    xStateMutex = xSemaphoreCreateMutex();

    if (xStateMutex != NULL)
    {
        xTaskCreate(vDisplayTask,
            "DisplayTask",
            configMINIMAL_STACK_SIZE,
            NULL,
            mainDISPLAY_TASK_PRIORITY,
            NULL);

        xTaskCreate(vPresenceTask,
            "PresenceTask",
            configMINIMAL_STACK_SIZE,
            NULL,
            mainPRESENCE_TASK_PRIORITY,
            NULL);

        xTaskCreate(vTimerTask,
            "TimerTask",
            configMINIMAL_STACK_SIZE,
            NULL,
            mainTIMER_TASK_PRIORITY,
            NULL);

        vTaskStartScheduler();
    }
    else
    {
        printf("Failed to create state mutex.\r\n");
    }

    for (; ; )
    {
    }
}
/*-----------------------------------------------------------*/

static const char* pcStateToString(int state)
{
    switch ((PomodoroState_t)state)
    {
    case STATE_IDLE:
        return "IDLE";

    case STATE_STUDYING:
        return "STUDYING";

    case STATE_PAUSED:
        return "PAUSED";

    case STATE_ALARM:
        return "ALARM";

    default:
        return "UNKNOWN";
    }
}
/*-----------------------------------------------------------*/

static BaseType_t xPomodoroRequestTransition(int newState, const char* caller)
{
    BaseType_t xResult = pdFAIL;

    if (xStateMutex == NULL)
    {
        taskENTER_CRITICAL();
        {
            printf("[KERNEL API] State mutex is NULL\r\n");
        }
        taskEXIT_CRITICAL();
        return pdFAIL;
    }

    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
    {
        PomodoroState_t oldState = gCurrentState;
        gCurrentState = (PomodoroState_t)newState;

        taskENTER_CRITICAL();
        {
            printf("[KERNEL API] State transition: %s -> %s by %s\r\n",
                pcStateToString(oldState),
                pcStateToString(gCurrentState),
                caller);
        }
        taskEXIT_CRITICAL();

        xSemaphoreGive(xStateMutex);
        xResult = pdPASS;
    }
    else
    {
        taskENTER_CRITICAL();
        {
            printf("[KERNEL API] Failed to take mutex\r\n");
        }
        taskEXIT_CRITICAL();
    }

    return xResult;
}
/*-----------------------------------------------------------*/

static void vDisplayTask(void* pvParameters)
{
    (void)pvParameters;

    for (; ; )
    {
        taskENTER_CRITICAL();
        {
            printf("[DisplayTask] Current State: %s\r\n", pcStateToString(gCurrentState));
        }
        taskEXIT_CRITICAL();

        vTaskDelay(mainDISPLAY_FREQUENCY_MS);
    }
}
/*-----------------------------------------------------------*/

static void vPresenceTask(void* pvParameters)
{
    (void)pvParameters;

    for (; ; )
    {
        vTaskDelay(mainABSENT_AFTER_MS);

        taskENTER_CRITICAL();
        {
            printf("[PresenceTask] User absent detected\r\n");
        }
        taskEXIT_CRITICAL();

        xPomodoroRequestTransition(STATE_PAUSED, "PresenceTask");

        vTaskDelay(mainRESUME_AFTER_MS);

        taskENTER_CRITICAL();
        {
            printf("[PresenceTask] User detected again\r\n");
        }
        taskEXIT_CRITICAL();

        xPomodoroRequestTransition(STATE_STUDYING, "PresenceTask");
    }
}
/*-----------------------------------------------------------*/

static void vTimerTask(void* pvParameters)
{
    (void)pvParameters;

    for (; ; )
    {
        vTaskDelay(mainALARM_AFTER_MS);

        taskENTER_CRITICAL();
        {
            printf("[TimerTask] Study time expired\r\n");
        }
        taskEXIT_CRITICAL();

        xPomodoroRequestTransition(STATE_ALARM, "TimerTask");
    }
}
/*-----------------------------------------------------------*/

void vBlinkyKeyboardInterruptHandler(int xKeyPressed)
{
    (void)xKeyPressed;
}