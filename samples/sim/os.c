#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <time.h>

/* printf("[ get_time %lu.%lu ]\n", ts.tv_sec, ts.tv_nsec); */

void get_time(uint32_t *sec, uint16_t *ms)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (sec != NULL) {
		*sec = ts.tv_sec;
	}

	if (ms != NULL) {
		*ms = ts.tv_nsec / 1000000LLU;
	}
}