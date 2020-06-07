// Handling of timers on linux systems
//
// Copyright (C) 2017-2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <time.h> // struct timespec
#include "autoconf.h" // CONFIG_CLOCK_FREQ
#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_from_us
#include "board/timer_irq.h" // timer_dispatch_many
#include "command.h" // DECL_CONSTANT
#include "internal.h" // console_sleep
#include "sched.h" // DECL_INIT


/****************************************************************
 * Timespec helpers
 ****************************************************************/

static uint32_t next_wake_time_counter;
static struct timespec next_wake_time;
static time_t start_sec;

// Compare two 'struct timespec' times
static inline uint8_t
timespec_is_before(struct timespec ts1, struct timespec ts2)
{
    return (ts1.tv_sec < ts2.tv_sec
            || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec < ts2.tv_nsec));
}

// Convert a 'struct timespec' to a counter value
static inline uint32_t
timespec_to_time(struct timespec ts)
{
    return ((ts.tv_sec - start_sec) * CONFIG_CLOCK_FREQ
            + ts.tv_nsec / NSECS_PER_TICK);
}

// Convert an internal time counter to a 'struct timespec'
static inline struct timespec
timespec_from_time(uint32_t time)
{
    int32_t counter_diff = time - next_wake_time_counter;
    struct timespec ts;
    ts.tv_sec = next_wake_time.tv_sec;
    ts.tv_nsec = next_wake_time.tv_nsec + counter_diff * NSECS_PER_TICK;
    if ((unsigned long)ts.tv_nsec >= NSECS) {
        if (ts.tv_nsec < 0) {
            ts.tv_sec--;
            ts.tv_nsec += NSECS;
        } else {
            ts.tv_sec++;
            ts.tv_nsec -= NSECS;
        }
    }
    return ts;
}

// Return the current time
static struct timespec
timespec_read(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

// Check if a given time has past
int
timer_check_periodic(struct timespec *ts)
{
    if (timespec_is_before(next_wake_time, *ts))
        return 0;
    *ts = next_wake_time;
    ts->tv_sec += 2;
    return 1;
}


/****************************************************************
 * Timers
 ****************************************************************/

// Return the current time (in clock ticks)
uint32_t
timer_read_time(void)
{
    return timespec_to_time(timespec_read());
}

// Activate timer dispatch as soon as possible
void
timer_kick(void)
{
    next_wake_time = timespec_read();
    next_wake_time_counter = timespec_to_time(next_wake_time);
}

// Invoke timers
static void
timer_dispatch(void)
{
    uint32_t next = timer_dispatch_many();
    next_wake_time = timespec_from_time(next);
    next_wake_time_counter = next;
}

void
timer_init(void)
{
    start_sec = timespec_read().tv_sec + 1;
    timer_kick();
}
DECL_INIT(timer_init);


/****************************************************************
 * Interrupt wrappers
 ****************************************************************/

void
irq_disable(void)
{
}

void
irq_enable(void)
{
}

irqstatus_t
irq_save(void)
{
    return 0;
}

void
irq_restore(irqstatus_t flag)
{
}

void
irq_wait(void)
{
    console_sleep(next_wake_time);
}

void
irq_poll(void)
{
    if (!timespec_is_before(timespec_read(), next_wake_time))
        timer_dispatch();
}
