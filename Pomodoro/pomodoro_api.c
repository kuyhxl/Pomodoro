#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "pomodoro_types.h"
#include "pomodoro_api.h"

static const BaseType_t xTransitionTable[POMO_STATE_COUNT][POMO_STATE_COUNT] = {
//              IDLE,   SETTING, STUDYING, PAUSED,  ALARM,   BREAK
/*IDLE*/     {pdFALSE, pdTRUE,  pdFALSE, pdFALSE, pdFALSE, pdFALSE},
/*SETTING*/  {pdTRUE,  pdFALSE, pdTRUE,  pdFALSE, pdFALSE, pdFALSE},
/*STUDYING*/ {pdTRUE,  pdFALSE, pdFALSE, pdTRUE,  pdTRUE,  pdFALSE},
/*PAUSED*/   {pdTRUE,  pdFALSE, pdTRUE,  pdFALSE, pdTRUE,  pdFALSE},
/*ALARM*/    {pdTRUE,  pdFALSE, pdFALSE, pdFALSE, pdFALSE, pdTRUE},
/*BREAK*/    {pdTRUE , pdFALSE, pdFALSE, pdFALSE, pdTRUE,  pdFALSE},
};

const char* pcPomodoroStateName(PomodoroState_t s) {
    switch (s) {
        case POMO_STATE_IDLE:       return "IDLE";
        case POMO_STATE_SETTING:    return "SETTING";
        case POMO_STATE_STUDYING:   return "STUDYING";
        case POMO_STATE_PAUSED:     return "PAUSED";
        case POMO_STATE_ALARM:      return "ALARM";
        case POMO_STATE_BREAK:      return "BREAK";
        default: return "UNKNOWN";
    }
    
}

PomodoroState_t xPomodoroGetState(void) {
    PomodoroState_t s;
    xSemaphoreTake(xStateMutex, portMAX_DELAY);
    s = xSession.state;
    xSemaphoreGive(xStateMutex);
    return s;
}

BaseType_t xPomodoroRequestTransition(PomodoroState_t new_state,
                                      const char* caller) {
    BaseType_t xResult = pdFALSE;
    PomodoroState_t old_state;

    if (caller == NULL) {
        caller = "unknown";
    }

    // 공유 세션 상태를 보호하기 위해 뮤텍스를 획득
    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) != pdTRUE) {
        return pdFALSE;
    }

    old_state = xSession.state;

    // 범위 확인
    if (new_state >= POMO_STATE_COUNT) {
        printf("[KERNEL API] Rejected: invalid state value (%d) by %s\r\n",
                (int) new_state, caller);
    }
    // FSM 유효성 검사
    else if (xTransitionTable[old_state][new_state] == pdFALSE) {
        printf("[KERNEL_API] Rejected: %s -> %s by %s (not allowed)\r\n",
                pcPomodoroStateName(old_state),
                pcPomodoroStateName(new_state),
                caller);
    }
    else {
        // 상태 전환 적용
        xSession.state = new_state;
        xResult = pdTRUE;

        printf("[KERNEL API] State transition: %s -> %s by %s\r\n",
                pcPomodoroStateName(old_state),
                pcPomodoroStateName(new_state),
                caller);
    }
    xSemaphoreGive(xStateMutex);
    return xResult;
}