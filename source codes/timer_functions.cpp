#include <windows.h>
#include <stdio.h>
#include "timer_functions.h"

//////////////////////////// timer functions/////////////////////////////////////
//use of
//stopWatch s;
//double t=0;
//startTimer(&s);
//stopTimer(&s);          
//t=getElapsedTime(&s);//in micro seconds

double getmsTime()
{
	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);
	LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
	return    (double)(ll_now / 10000);//millie seconds;                                 
}

double getusTime()
{
	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);
	LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
	return    (double)(ll_now / 10);//micro seconds;                                      
}

void startTimer(stopWatch *timer) {
	QueryPerformanceCounter(&timer->start);
}

void stopTimer(stopWatch *timer) {
	QueryPerformanceCounter(&timer->stop);
}

double LIToMicroSecs(LARGE_INTEGER * L) {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return ((double)L->QuadPart / ((double)frequency.QuadPart / 1000000));//000000 micro   000 millie (must be micro for frame timing)
}

double getElapsedTime(stopWatch *timer) {
	LARGE_INTEGER time;
	time.QuadPart = timer->stop.QuadPart - timer->start.QuadPart;
	return LIToMicroSecs(&time);
}

void CALLBACK TimerFunction(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	static int timer = 0;
	static int count = 0;
	static int Flag = 0;
	static double tt = 0;
	static stopWatch st;
	timer++;
	if (Flag == 0) {
		stopTimer(&st);
		tt = getElapsedTime(&st);//in micro seconds
		printf("%.0f\n", tt);

		startTimer(&st);
		if (timer == 40) {
			// Flag=1;
			printf("count %d\n  ", count++);
			timer = 0;
		}
	}
}