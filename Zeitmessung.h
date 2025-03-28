#ifndef ZEITMESSUNG_H
#define ZEITMESSUNG_H

#ifdef __unix__
#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stddef.h>
#endif // __unix__

#ifdef _WIN32
#include <time.h>
#include <windows.h>
#endif // _WIN32

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif // __MACH__

typedef long long unsigned t_timevalue;

struct t_stopwatch
{
    t_timevalue  TimestampStart;
    t_timevalue  ResultingTime;
    int isRunning;
};

typedef struct t_stopwatch t_stopwatch;


t_timevalue get_posix_clock_time();
void startStopwatch(t_stopwatch* theWatch);
t_timevalue restartStopwatch(t_stopwatch* theWatch);
t_timevalue stopStopwatch(t_stopwatch* theWatch);
t_timevalue readStopwatch(t_stopwatch* theWatch);
t_timevalue evaluateStopwatch(t_stopwatch* theWatch);


#endif // ZEITMESSUNG_H
