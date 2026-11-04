#ifndef POMODORO_API_H
#define POMODORO_API_H

#include "FreeRTOS.h"
#include "pomodoro_types.h"

BaseType_t xPomodoroRequestTransition( PomodoroState_t new_state,
                                       const char * caller);

PomodoroState_t xPomodoroGetState(void);

const char* pcPomodoroStateName(PomodoroState_t s);

#endif /* POMODORO_API_H*/
