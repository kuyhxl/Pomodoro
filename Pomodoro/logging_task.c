#include <stdio.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"
#include "logging_task.h"

#define LOG_FILE_NAME "pomodoro_log.txt"

static uint32_t s_session_index = 0U;

static void prvAppendLog(PomodoroSession_t snapshot) {
    FILE* fp;
    time_t now;
    struct tm* lt;
    char timestamp[32];

    fp = fopen(LOG_FILE_NAME, "a");
    if (fp == NULL) {
        printf("[LoggingTask] WARN: cannoot open %s for append.\r\n", LOG_FILE_NAME);
        return;
    }

    now = time(NULL);
    lt = localtime(&now);
    if (lt != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %h:%M:%S", lt);
    }
    else {
        snprintf(timestamp, sizeof(timestamp), "????-??-?? ??:??:??");
    }

    fprintf(fp,
            "[%s] session=%u state=%s done=%u "
            "study_total=%u study_acc=%u break_total=%u break_acc=%u\n",
            timestamp,
            (unsigned) s_session_index,
            pcPomodoroStateName(snapshot.state),
            (unsigned) snapshot.completed_pomodoros,
            (unsigned) snapshot.study_seconds_total,
            (unsigned) snapshot.study_seconds_accumulated,
            (unsigned) snapshot.break_seconds_total,
            (unsigned) snapshot.break_seconds_accumulated
        );
    fclose(fp);
}

static void prvSimulateExternalSync(PomodoroSession_t snapshot) {
    printf("[LoggingTask] >>> Sync to external app:\r\n");
    printf("[LoggingTask]     session #%u, %s phase ended\r\n",
             (unsigned) s_session_index,
             (snapshot.last_completed == LAST_COMPLETED_STUDY) ? "STUDY" :
             (snapshot.last_completed == LAST_COMPLETED_BREAK) ? "BREAK" : "OTHER"
           );
    printf("[LoggingTask]     pomodoros done=%u total study acc=%u sec\r\n",
             (unsigned) snapshot.completed_pomodoros,
             (unsigned) snapshot.study_seconds_accumulated
           ); 
    printf("[LoggingTask] <<< Sync complete.\r\n");
}

void vLoggingSyncTask(void* pvParameters) {
    PomodoroSession_t snapshot;

    (void)pvParameters;

    printf("[LoggingTask] started. Logging to %s on each ALARM.\r\n", LOG_FILE_NAME);

    for (;; ) {
        if (xSemaphoreTake(xAlarmSemaphore, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        s_session_index++;

        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        snapshot = xSession;
        xSemaphoreGive(xStateMutex);

        prvAppendLog(snapshot);
        prvSimulateExternalSync(snapshot);
    }

}