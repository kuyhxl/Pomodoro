#ifndef POMODORO_MAIN_H
#define POMODORO_MAIN_H

#include <windows.h>

void vStartPomodoroSystem(void);

int iGetLastKey(void);
HANDLE xGetKeyboardEvent(void);

#endif /* POMODORO_MAIN_H */

// Factory defaults (d key)
#define POMO_DEFAULT_STUDY_SECONDS 25U
#define POMO_DEFAULT_BREAK_SECONDS 5U