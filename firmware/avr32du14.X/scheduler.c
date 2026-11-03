/*
 * scheduler.c - Simple time-triggered cooperative scheduler
 * Target: AVR64DU (8-bit AVR), plain standard C.
 *
 * - sysTick is incremented elsewhere (timer interrupt, every 10 ms).
 * - Tasks run to completion from the main loop (cooperative, no preemption).
 * - The task table is owned by the application and registered with
 *   schedulerInit(); tasks are identified by their index in that table.
 * - All times are in ticks (1 tick = 10 ms).
 *
 * Time limits: tick arithmetic uses uint16_t with wrap-around-safe compares,
 * so any delay or period must be <= 32767 ticks (~327 s).
 */

#include <stdint.h>
#include <stddef.h>
#include "scheduler.h"

static schedulerTask *tasks = NULL;
static uint8_t        taskCount = 0;

/* ---------------------------------------------------------------------- */
/* Helpers                                                                */
/* ---------------------------------------------------------------------- */

/* 16-bit read is not atomic on an 8-bit AVR: read until two reads agree. */
static uint16_t getTick(void)
{
    uint16_t a, b;
    do {
        a = sysTick;
        b = sysTick;
    } while (a != b);
    return a;
}

/* True if 'now' has reached or passed 'target' (wrap-around safe). */
static uint8_t timeReached(uint16_t now, uint16_t target)
{
    return (int16_t)(now - target) >= 0;
}

/* ---------------------------------------------------------------------- */
/* Scheduler API (callable from tasks or main)                            */
/* ---------------------------------------------------------------------- */

void schedulerInit(schedulerTask *table, uint8_t count)
{
    uint8_t i;
    uint16_t now = getTick();

    tasks = table;
    taskCount = count;

    if (tasks == NULL) {
        taskCount = 0;
        return;
    }

    for (i = 0; i < taskCount; i++) {
        tasks[i].remaining = 0;
        if (tasks[i].state == schedulerTaskEnabled) {
            tasks[i].nextRun = (uint16_t)(now + tasks[i].nextRun);
        }
    }
}

void schedulerTaskStart(uint8_t id, uint16_t delay, uint16_t period)
{
    if (id >= taskCount) return;
    tasks[id].period  = period;
    tasks[id].nextRun = (uint16_t)(getTick() + delay);
    tasks[id].state   = schedulerTaskEnabled;
}

void schedulerTaskStop(uint8_t id)
{
    if (id >= taskCount) return;
    tasks[id].state = schedulerTaskDisabled;
}

void schedulerTaskPause(uint8_t id)
{
    if (id >= taskCount) return;
    if (tasks[id].state != schedulerTaskEnabled) return;

    uint16_t now = getTick();
    tasks[id].remaining = timeReached(now, tasks[id].nextRun)
                              ? 0
                              : (uint16_t)(tasks[id].nextRun - now);
    tasks[id].state = schedulerTaskPaused;
}

void schedulerTaskResume(uint8_t id)
{
    if (id >= taskCount) return;
    if (tasks[id].state != schedulerTaskPaused) return;

    tasks[id].nextRun = (uint16_t)(getTick() + tasks[id].remaining);
    tasks[id].state   = schedulerTaskEnabled;
}

/* ---------------------------------------------------------------------- */
/* Scheduler core - call repeatedly from main()                           */
/* ---------------------------------------------------------------------- */
void schedulerRun(void)
{
    uint8_t i;
    uint16_t now = getTick();

    for (i = 0; i < taskCount; i++) {
        schedulerTask *t = &tasks[i];

        if (t->state == schedulerTaskEnabled && timeReached(now, t->nextRun)) {
            /* Update schedule BEFORE calling, so the task may safely
             * start/stop/pause itself or others from inside its body. */
            if (t->period == SCHEDULERONESHOT) {
                t->state = schedulerTaskDisabled;                  /* one-shot */
            } else if (t->period == SCHEDULERALWAYS) {
                t->nextRun = now;                                  /* due again next pass */
            } else {
                t->nextRun = (uint16_t)(t->nextRun + t->period);   /* no drift */
            }
            t->func();
        }
    }
}
