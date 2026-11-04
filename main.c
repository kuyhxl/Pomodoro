#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <Windows.h>

// FreeRTOS Kernel
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Pomodoro project entry point
#include "pomodoro_main.h"

// Interrupt numberr used for keyboard
#define mainINTERRUPT_NUMBER_KEYBOARD 3

// most recent keystroke
static volatile int g_iLastKey = 0;

// Win32 Handle
static HANDLE g_hKeyboardEvent = NULL;

// Win32 Thread
static DWORD WINAPI prvWindowsKeyboardInputThread(void* pvParameter) {
    (void)pvParameter;

    for (;; ) {
        if (_kbhit()) {
            g_iLastKey = _getch();
            if (g_hKeyboardEvent != NULL) {
                SetEvent(g_hKeyboardEvent);
            }
        }
        Sleep(20);
    }

}

int iGetLastKey(void) { int k = g_iLastKey; g_iLastKey = 0; return k; }
HANDLE xGetKeyboardEvent(void) { return g_hKeyboardEvent; }

int main(void) {
    g_hKeyboardEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    CreateThread(NULL, 0,
                (LPTHREAD_START_ROUTINE)prvWindowsKeyboardInputThread,
                NULL, 0, NULL);

    vStartPomodoroSystem();

    return 0;
}
/* ========================================================================== 
* FreeRTOS application hooks (required by the Windows port) 
* ========================================================================== */ 

void vApplicationMallocFailedHook(void) {
    /* Called if pvPortMalloc() returns NULL. Most often = heap too small. */ 
    printf("[HOOK] Malloc failed!\r\n"); 
    configASSERT(0); 
} 

void vApplicationIdleHook(void) { 
    /* Idle hook runs whenever no task is ready. Keep it cheap. * Sleep slightly to avoid pegging the host CPU at 100%. */
    Sleep(1); 
} 

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName) {
    (void)pxTask; 
    printf("[HOOK] Stack overflow in task: %s\r\n", pcTaskName);
    configASSERT(0); 
} 

void vApplicationTickHook(void) { 
    /* Tick hook runs in the timer ISR context every tick. Keep empty. */
} 

void vApplicationDaemonTaskStartupHook(void) { 
    /* Called once after the scheduler has started, from the timer 
    * service task's context. We don't need anything here. */ 
} 

/* configASSERT() routes here. */ 
void vAssertCalled(unsigned long ulLine, const char* const pcFileName) { 
    static BaseType_t xPrinted = pdFALSE; 
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0; 
    /* Prevent recursion if printf itself asserts. */ 
    taskENTER_CRITICAL();
    { 
        if (xPrinted == pdFALSE) {
            xPrinted = pdTRUE; 
            printf("[ASSERT] %s:%lu\r\n", pcFileName, ulLine); 
            fflush(stdout); 
        } 
        /* Spin so a debugger can attach and inspect. */ 
        while (ulSetToNonZeroInDebuggerToContinue == 0) { 
            __nop(); 
            __nop(); 
        } 
    } 
    taskEXIT_CRITICAL(); 
}
