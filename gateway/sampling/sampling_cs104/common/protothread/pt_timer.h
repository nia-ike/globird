#ifndef _PT_TIMER_H_
#define _PT_TIMER_H_
#include <stdint.h>


struct timer { 
	int64_t start;
	int interval; 
};


int timer_expired(struct timer *t);
void timer_set(struct timer *t, int interval);



#endif
