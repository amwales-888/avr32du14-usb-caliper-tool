/*
 * scheduler.h - Simple time-triggered cooperative scheduler
 * Target: AVR64DU (8-bit AVR), plain standard C.
 *
 * 1 tick = 10 ms (sysTick is incremented by the timer interrupt).
 * Delays and periods must be <= 32767 ticks (~327 s).
 *
 * The application owns the task table (see main.c) and hands it to
 * schedulerInit(). Tasks are identified by their index in that table.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/* Tick counter, incremented every 10 ms by the timer interrupt.
 * Defined in your timer/ISR source file. */
extern volatile uint16_t sysTick;

/* Duration of one tick in milliseconds (timer interrupt period). */
#define SCHEDULERTICKMS      10UL

/* Convert milliseconds / seconds to ticks (rounded to the nearest tick).
 * Use for delays, periods and the initial nextRun value in the task table.
 * Result must be <= 32767 ticks (~327 s). */
#define SCHEDULERMS(ms)      ((uint16_t)(((ms) + (SCHEDULERTICKMS / 2UL)) / SCHEDULERTICKMS))
#define SCHEDULERS(s)        SCHEDULERMS((s) * 1000UL)

/* Special period values */
#define SCHEDULERONESHOT     0u       /* run once, then the task disables itself */
#define SCHEDULERALWAYS      0xFFFFu  /* run on every pass of schedulerRun()
                                         (no delay between calls)               */

typedef enum {
    schedulerTaskDisabled = 0,  /* not scheduled                                */
    schedulerTaskEnabled,       /* scheduled, runs when nextRun is reached      */
    schedulerTaskPaused         /* frozen; remaining time kept until resumed    */
} schedulerTaskState;

typedef void (*schedulerTaskFunc)(void);

typedef struct {
    schedulerTaskFunc  func;      /* function to call                           */
    uint16_t           nextRun;   /* in the initial table: delay in ticks before
                                     the first run; afterwards: sysTick value
                                     of the next run (managed by the scheduler) */
    uint16_t           period;    /* ticks between runs, or SCHEDULERONESHOT /
                                     SCHEDULERALWAYS                            */
    uint16_t           remaining; /* ticks left when paused (scheduler use)     */
    schedulerTaskState state;     /* initial state of the task                  */
} schedulerTask;

/* Register the task table. The table must stay valid for the whole program
 * (use a static array). Tasks that start out enabled have their nextRun
 * interpreted as a delay and converted to an absolute tick. */
void schedulerInit(schedulerTask *table, uint8_t count);

/* Start (or restart) a task: first run after 'delay' ticks, then every
 * 'period' ticks. Use SCHEDULERONESHOT to run once, or SCHEDULERALWAYS to
 * run on every pass of schedulerRun(). Example:
 *     schedulerTaskStart(id, SCHEDULERMS(250), SCHEDULERS(2)); */
void schedulerTaskStart(uint8_t id, uint16_t delay, uint16_t period);

/* Stop a task completely. */
void schedulerTaskStop(uint8_t id);

/* Pause an enabled task, remembering the time left until its next run. */
void schedulerTaskPause(uint8_t id);

/* Resume a paused task with the time that was left. */
void schedulerTaskResume(uint8_t id);

/* Run all tasks that are due. Call repeatedly from the main loop. */
void schedulerRun(void);

#endif /* SCHEDULER_H */
