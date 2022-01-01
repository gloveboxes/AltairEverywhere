#include "Delay.h"

void usleep(long usec)
{
	struct timespec req;
	struct timespec rem;

	req.tv_sec = usec / 1000000;
	req.tv_nsec = (usec % 1000000) * 1000;

	while (nanosleep(&req, &rem) != 0)
	{
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}
}