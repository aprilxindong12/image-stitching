#pragma once
#ifndef TIMER_FUNCTIONS_H
#define TIMER_FUNCTIONS_H
#include <windows.h>

typedef struct {
	LARGE_INTEGER start;
	LARGE_INTEGER stop;
} stopWatch;
double getmsTime();
double getusTime();
void startTimer(stopWatch *timer);
void stopTimer(stopWatch *timer);
double getElapsedTime(stopWatch *timer);
void CALLBACK TimerFunction(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

#endif
