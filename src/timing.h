#ifndef SREND_TIMING_H_
#define SREND_TIMING_H_

#include <stdint.h>
#if defined(__unix__) || defined(__unix) || defined(__APPLE__)&&defined(__MACH__)
	#include <time.h>
#elif defined(_WIN32)
	#include <windows.h>
#endif

// Opaque struct because implementation details are platform specific.
// Can't guarentee anything about internal variables.
//
// Microsecond resolution timer. Call start_timer to start, mark_timer
// to save the time called and get_elapsed_time to get the time between the
// start and latest mark.
struct Timer;

// Allocate memory for the timer and return a reference.
struct Timer *create_timer(void);

// Free the timer. Just calling free.
void destroy_timer(struct Timer *t);

// Restart the timer.
void start_timer(struct Timer *t);

// Mark the timer. Equivalent to Lap.
void mark_timer(struct Timer *t);

// Get the time between the start and end timer function calls.
// Make SURE to mark the timer at least once before calling this function!!!
uint64_t get_elapsed_time(struct Timer *t);


/*
*   Sleep
*/
void us_sleep(uint64_t ms_time);

#endif
