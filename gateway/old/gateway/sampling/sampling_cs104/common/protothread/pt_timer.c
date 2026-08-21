#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include "pt_timer.h"


#define SCLOCK_GEQ(a, b)	((unsigned long)((a) - (b)) < \
				((unsigned long)(~((unsigned long)0)) >> 1))


static int64_t clock_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/*---------------------------------------------------------------------------*/
/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * \param t A pointer to the timer
 *
 * \return Non-zero if the timer has expired, zero otherwise.
 *
 */
int timer_expired(struct timer *t)
{ 
	return (uint64_t)(clock_time() - t->start) >= (int)t->interval; 
}


/*---------------------------------------------------------------------------*/
/**
 * Set a timer.
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function stimer_expired() will evaluate to true after
 * the timer has expired.
 *
 * \param t A pointer to the timer
 * \param interval The interval before the timer expires.
 *
 */
void timer_set(struct timer *t, int interval)
{ 
	t->interval = interval; 
	t->start = clock_time(); 
}


/*---------------------------------------------------------------------------*/
/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the stimer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * stimer_restart() function.
 *
 * \param t A pointer to the timer.
 *
 * \sa stimer_restart()
 */
void timer_reset(struct timer *t)
{
  t->start += t->interval;
}