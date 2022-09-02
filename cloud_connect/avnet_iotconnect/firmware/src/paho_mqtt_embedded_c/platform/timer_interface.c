/**
 * Copyright 2019-2020 EBV Elektronik. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of EBV Elektronik may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EBV "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL EBV BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

/** Paho MQTT includes. */
#include "timer_interface.h"

extern volatile uint32_t ms_ticks;

#define	timer_add(tvp, uvp, vvp)	                        \
	do                                                      \
    {                                                       \
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;      \
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;   \
		if ((vvp)->tv_usec >= 1000000)                      \
        {                                                   \
			(vvp)->tv_sec++;								\
			(vvp)->tv_usec -= 1000000;						\
		}													\
	} while (0)

#define	timer_subtract(tvp, uvp, vvp)                       \
	do                                                      \
    {                                                       \
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
		if ((vvp)->tv_usec < 0)                             \
        {                                                   \
			(vvp)->tv_sec--;                                \
			(vvp)->tv_usec += 1000000;                      \
		}                                                   \
	} while (0)

/**
 * \brief Initialize time object with current time.
 *
 * \param[out] timer The timer to be initialized.
 */
int get_time_of_day(struct timeval *time)
{
	uint32_t tick_ms = 0;

	if (time == NULL) {
        return -1;
    }

	tick_ms = ms_ticks;
	time->tv_sec =  (tick_ms / 1000);
	time->tv_usec = ((tick_ms % 1000) * 1000);

	return 0;
}

/**
 * \brief Clear timer data.
 *
 * \param[out] timer The timer to be cleared.
 */
void TimerInit(Timer *timer)
{
    if (timer == NULL) {
        return;
    }
    
    memset(timer, 0, sizeof(struct mqtt_timer));
}

/**
 * \brief Check if a timer has expired.
 *
 * \param[in] timer The timer to be checked for expiration.
 *
 * \return 1 if the timer has expired, 0 otherwise.
 */
char TimerIsExpired(Timer *timer)
{
	struct timeval time_now = {0};
	struct timeval time_result = {0};

    if (timer == NULL) {
        return 1;
    }

	get_time_of_day(&time_now);
    
	timer_subtract(&timer->end_time, &time_now, &time_result);

	return (time_result.tv_sec < 0 || (time_result.tv_sec == 0 && time_result.tv_usec <= 0));
}

/**
 * \brief Create a timer set to expire in the specified number of milliseconds.
 *
 * \param[out] timer The timer set to expire.
 * \param[in] timeout_ms The timer expiration delay in milliseconds.
 */
void TimerCountdownMS(Timer *timer, unsigned int timeout_ms)
{
	struct timeval time_now = {0};
    struct timeval time_interval = {timeout_ms / 1000, (int)((timeout_ms % 1000) * 1000)};

    if (timer == NULL) {
        return;
    }

	get_time_of_day(&time_now);
    
	timer_add(&time_now, &time_interval, &timer->end_time);
}

/**
 * \brief Create a timer set to expire in the specified number of seconds.
 *
 * \param[out] timer The timer set to expire.
 * \param[in] timeout_ms The timer expiration delay in seconds.
 */
void TimerCountdown(Timer *timer, unsigned int timeout)
{
	struct timeval time_now = {0};
	struct timeval time_interval = {timeout, 0};

    if (timer == NULL)
    {
        return;
    }

	get_time_of_day(&time_now);

	timer_add(&time_now, &time_interval, &timer->end_time);
}

/**
 * \brief Check the time remaining on a given timer.
 *
 * \param[in] timer The timer to be checked.
 *
 * \return The number of milliseconds left on the timer.
 */
int TimerLeftMS(Timer *timer)
{
	int result_ms = 0;
	struct timeval time_now = {0};
    struct timeval time_result = {0};

    if (timer == NULL)
    {
        return 0;
    }

	get_time_of_day(&time_now);
    
	timer_subtract(&timer->end_time, &time_now, &time_result);
	if(time_result.tv_sec >= 0)
    {
		result_ms = (int)((time_result.tv_sec * 1000) + (time_result.tv_usec / 1000));
	}

	return result_ms;
}
