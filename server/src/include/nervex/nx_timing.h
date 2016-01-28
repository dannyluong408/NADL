/* Generic portable timing utilities
 * All functions are prefixed with nx_
 * 
 * Sleep functions require pthreads.h
 */

#ifndef __NX_TIMING_H
#define __NX_TIMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef __WIN32
#include <Windows.h>
uint64_t nx_get_highres_time_us() {
	LARGE_INTEGER freq;
	LARGE_INTEGER time;
	/* This function will need to be modified to run on pre-winxp machines, or not. */
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&time); 
	return (time.QuadPart * 1000000.0 / freq.QuadPart);
}
#elif defined (__linux)
#include <sys/sysinfo.h> 
#include <sys/time.h>
#include <time.h>
uint64_t nx_get_highres_time_us() {
	struct timespec ts;
	const int error = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	if (error) {
		// trouble
		return 0;
	}
	const uint64_t return_val = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
	return return_val;
}
/* Untested - need a mac to test on */
#elif defined (__MACOSX__)
#include <sys/time.h>
uint64_t nx_get_highres_time_us() {
	timeval time;
	gettimeofday(&time, NULL);
	uint64_t output_us = time.tv_sec * 1000000.0; /* seconds -> us */
	output_us += time.tv_usec;
	return output_us;
}
#endif

/* Generic OS-independent functions. Requires pthreads. */
#include <pthread.h>
#include <time.h>

/* Sleep for uint64_t nanoseconds */
void nx_nsleep(const uint64_t nanoseconds) {
	struct timespec req,rem;
	if (nanoseconds > 999999999) {
		req.tv_nsec = nanoseconds%1000000000;
		req.tv_sec = (nanoseconds - req.tv_nsec) / 1000000000;
	} else {
		req.tv_sec = 0;
		req.tv_nsec = nanoseconds;
	}
	if (nanosleep(&req,&rem) == -1) {
		uint64_t ns = rem.tv_nsec + (rem.tv_sec * 1000000000);
		nx_nsleep(ns);
	}
}

/* Sleep for uint64_t microseconds */
void nx_usleep(const uint64_t microseconds) {
	nx_nsleep(1000 * microseconds);
}

/* Sleep for uint64_t milliseconds */
void nx_msleep(const uint64_t milliseconds) {
	nx_nsleep(1000 * 1000 * milliseconds);
}

#ifdef __cplusplus
}
#endif

/* __NX_TIMING_H */
#endif 