/*
 * button.c - Simple button debouncer driven by the scheduler
 * Target: AVR64DU (8-bit AVR), plain standard C.
 *
 * See button.h for usage and state semantics.
 */

#include <stdint.h>
#include <stddef.h>
#include "button.h"

static button *buttons = NULL;
static uint8_t buttonCount = 0;

void buttonInit(button *table, uint8_t count)
{
    uint8_t i;

    buttons = table;
    buttonCount = count;

    if (buttons == NULL) {
        buttonCount = 0;
        return;
    }

    for (i = 0; i < buttonCount; i++) {
        uint8_t level = 0;

        if (buttons[i].read != NULL) {
            level = (buttons[i].read() != 0);
        }
        buttons[i].level      = level;
        buttons[i].counter    = 0;
        buttons[i].state      = level ? buttonStateHeld : buttonStateIdle;
        buttons[i].heldMs     = 0;
        buttons[i].nextHeldMs = BUTTONHELDREPEATMS;
    }
}

void buttonTask(void)
{
    uint8_t i;

    for (i = 0; i < buttonCount; i++) {
        button *b = &buttons[i];
        uint8_t raw;

        if (b->read == NULL) continue;

        /* One-poll states move on first. */
        if (b->state == buttonStatePressed) {
            b->state = buttonStateHeld;
        } else if (b->state == buttonStateReleased) {
            b->state = buttonStateIdle;
        }

        /* Time passes while the debounced level is "pressed". Counting
         * before sampling means the poll that accepts the release is
         * included, and the poll that accepts the press is not. */
        if (b->level) {
            b->heldMs += BUTTONPOLLMS;
        }

        raw = (b->read() != 0);

        if (raw == b->level) {
            b->counter = 0;                 /* input agrees: restart count */
        } else if (++b->counter >= BUTTONDEBOUNCESAMPLES) {
            b->counter = 0;                 /* change is stable: accept it */
            b->level   = raw;

            /* State is set before the callback so it can query it. */
            if (raw) {
                b->state      = buttonStatePressed;
                b->heldMs     = 0;
                b->nextHeldMs = BUTTONHELDREPEATMS;
                if (b->onPressed != NULL) b->onPressed(i);
            } else {
                b->state = buttonStateReleased;
                if (b->onReleased != NULL) b->onReleased(i, b->heldMs);
            }
        }

        /* Periodic held notification. Only while stably held, so it never
         * fires on the poll that accepted a press or a release. The next
         * time is advanced by the interval, so it stays correct even if
         * the poll period does not divide the interval evenly. */
        if (b->state == buttonStateHeld && b->heldMs >= b->nextHeldMs) {
            b->nextHeldMs += BUTTONHELDREPEATMS;
            if (b->onHeld != NULL) b->onHeld(i, b->heldMs);
        }
    }
}

buttonState buttonGetState(uint8_t id)
{
    if (id >= buttonCount) return buttonStateIdle;
    return buttons[id].state;
}
