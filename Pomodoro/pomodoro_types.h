#ifndef POMODORO_TYPES_H
#define POMODORO_TYPES_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

// FSM States
typedef enum {
    POMO_STATE_IDLE = 0,
    POMO_STATE_SETTING,
    POMO_STATE_STUDYING,
    POMO_STATE_PAUSED,
    POMO_STATE_ALARM, 
    POMO_STATE_BREAK,
    POMO_STATE_COUNT
} PomodoroState_t;

// Event types
typedef enum {
    EVT_USER_START = 0,
    EVT_USER_STOP,
    EVT_USER_SET_TIME,
    EVT_USER_SET_BREAK_TIME,
    EVT_USER_QUIT,
    EVT_PRESENCE_ABSENT,
    EVT_PRESENCE_PRESENT,
    EVT_TIMER_EXPIRED,
    EVT_ALARM_ACK,

    EVT_USER_RESET,
    EVT_USER_DEFAULTS,
    EVT_USER_CLEAR_COUNTER,
} PomodoroEventType_t;

// 어떤 단계(study/break)가 끝났는지 구분하기 위한 플래그
typedef enum {
    LAST_COMPLETED_NONE = 0,
    LAST_COMPLETED_STUDY,
    LAST_COMPLETED_BREAK,

}LastCompleted_t;

// 작업 관리자가 큐에서 꺼내 처리하는 이벤트 구조체
typedef struct {
    PomodoroEventType_t type;
    uint32_t    payload;
} PomodoroEvent_t;

// 전역 공유 상태 구조체
typedef struct {
    PomodoroState_t state;
    uint32_t    study_seconds_total;
    uint32_t    study_seconds_remaining;
    uint32_t    study_seconds_accumulated;
    uint32_t    break_seconds_total;
    uint32_t    break_seconds_remaining;
    uint32_t    break_seconds_accumulated;
    uint32_t    completed_pomodoros;
    LastCompleted_t last_completed;
} PomodoroSession_t;

// 다른 파일에서 접근할 전역 RTOS 객체 핸들
extern QueueHandle_t xEventQueue;
extern SemaphoreHandle_t xStateMutex;
extern SemaphoreHandle_t xAlarmSemaphore;
extern PomodoroSession_t xSession;

#endif /* POMODORO_TYPES_H */