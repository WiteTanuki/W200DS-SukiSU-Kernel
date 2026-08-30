#ifndef __LINUX_TIME64_H_
#define __LINUX_TIME64_H_

typedef long long time64_t;
typedef unsigned long long timeu64_t;


struct timespec64 {
	time64_t	tv_sec;			/* seconds */
	long		tv_nsec;		/* nanoseconds */
};

struct itimerspec64 {
	struct timespec64 it_interval;
	struct timespec64 it_value;
};

#endif