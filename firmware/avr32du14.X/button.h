/*
 * button.h - Simple button debouncer driven by the scheduler
 * Target: AVR64DU (8-bit AVR), plain standard C.
 *
 * Usage:
 *   1. Write a read function per button returning non-zero while the button
 *      is physically pressed (handle active-low wiring inside it).
 *   2. Define a static table of button entries using BUTTONINIT().
 *   3. Call buttonInit(table, count) once at startup.
 *   4. Add buttonTask to the scheduler task table with period
 *      BUTTONPOLLPERIOD (see main.c).
 *
 * Debouncing: the raw input must differ from the current stable level for
 * BUTTONDEBOUNCESAMPLES consecutive polls before the change is accepted.
 *
 * States (one transition per poll):
 *   Idle -> Pressed -> Held -> Released -> Idle
 * Pressed and Released each last for exactly one poll period (10 ms by
 * default). A task that checks buttonGetState() less often than that can
 * miss them, so use the callbacks when you must not miss an event.
 *
 * Callbacks (all optional, pass NULL to skip):
 *   onPressed   - once, when a press is accepted.
 *   onHeld      - every BUTTONHELDREPEATMS while the button stays down,
 *                 first call after BUTTONHELDREPEATMS. Receives the total
 *                 time held so far in ms.
 *   onReleased  - once, when a release is accepted. Receives the total time
 *                 the button was held in ms.
 *
 * Held time is measured between the accepted press and the accepted
 * release. Both are delayed by the same debounce time, so the result
 * matches the real press duration. A button already down at start-up is
 * timed from buttonInit().
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "scheduler.h"

/* Poll period and debounce time in milliseconds. */
#define BUTTONPOLLMS            10UL
#define BUTTONDEBOUNCEMS        30UL

/* Interval between onHeld callbacks in milliseconds. */
#define BUTTONHELDREPEATMS      500UL

/* Number of consecutive differing samples needed to accept a change. */
#define BUTTONDEBOUNCESAMPLES   ((uint8_t)((BUTTONDEBOUNCEMS + BUTTONPOLLMS - 1UL) / BUTTONPOLLMS))

/* Scheduler period to use for buttonTask. */
#define BUTTONPOLLPERIOD        SCHEDULERMS(BUTTONPOLLMS)

typedef enum {
    buttonStateIdle = 0,   /* released and stable                         */
    buttonStatePressed,    /* just pressed (one poll period)              */
    buttonStateHeld,       /* pressed and stable                          */
    buttonStateReleased    /* just released (one poll period)             */
} buttonState;

/* Callback without time: 'id' is the index of the button in the table
 * passed to buttonInit(). Called from buttonTask, so keep it short. */
typedef void (*buttonCallback)(uint8_t id);

/* Callback with time: 'heldMs' is the time the button has been held (onHeld)
 * or was held in total (onReleased), in milliseconds. */
typedef void (*buttonTimedCallback)(uint8_t id, uint32_t heldMs);

/* Returns non-zero while the button is physically pressed. */
typedef uint8_t (*buttonReadFunc)(void);

typedef struct {
    buttonReadFunc      read;        /* required                                 */
    buttonCallback      onPressed;   /* optional, may be NULL                    */
    buttonTimedCallback onHeld;      /* optional, may be NULL                    */
    buttonTimedCallback onReleased;  /* optional, may be NULL                    */
    buttonState         state;       /* managed by the module                    */
    uint8_t             level;       /* debounced level, managed by the module   */
    uint8_t             counter;     /* debounce counter, managed by the module  */
    uint32_t            heldMs;      /* time held so far, managed by the module  */
    uint32_t            nextHeldMs;  /* time of next onHeld, managed by module   */
} button;

/* Initialise a table entry. Example:
 *     { BUTTONINIT(readButton0, onPressed0, onHeld0, onReleased0) },    */
#define BUTTONINIT(readFunc, pressedCb, heldCb, releasedCb) \
    (readFunc), (pressedCb), (heldCb), (releasedCb), buttonStateIdle, 0, 0, 0, 0

/* Register the button table (must stay valid for the whole program). The
 * current input level is sampled, so a button already held at start-up
 * begins in buttonStateHeld without generating a pressed callback. */
void buttonInit(button *table, uint8_t count);

/* Scheduler task: polls and debounces all buttons. Run it every
 * BUTTONPOLLPERIOD ticks. */
void buttonTask(void);

/* Current state of button 'id' (buttonStateIdle for an invalid id). */
buttonState buttonGetState(uint8_t id);

#endif /* BUTTON_H */
