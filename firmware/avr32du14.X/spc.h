/**
 * spc.h - Mitutoyo Digimatic (SPC) receiver for AVR DU (AVR32DU28 etc.)
 *
 * Protocol, as implemented by https://github.com/Roger-random/mitutoyo :
 *   - 13 nibbles (52 bits) per frame, each nibble sent LSB first.
 *   - DATA is sampled on the falling edge of CLK (CLK idles high).
 *   - Frame starts when REQ is asserted at the instrument (pulled low); REQ may
 *     be released again once the first nibble has been clocked out.
 *
 *     nibble  0..3   header, always 0xF
 *     nibble  4      sign: 0x0 positive, 0x8 negative
 *     nibble  5..10  six decimal digits, most significant first
 *     nibble  11     number of digits after the decimal point
 *     nibble  12     unit: 0 = mm, non-zero = inch
 *
 * How it works (CPU cost):
 *   Nothing busy-waits.  A falling-edge interrupt on the CCLK pin calls a tiny
 *   callback (sample CDATA, shift into a nibble).  spcRead() is a non-blocking
 *   state machine: call it repeatedly from your main loop.
 *
 * MCC Harmony / pins.h requirements:
 *   - Pins are named in MCC as CCLK (input), CDATA (input) and CREQ (output),
 *     and the driver uses the macros generated in pins.h for them.
 *   - CCLK must have interrupt-on-change enabled in the MCC Pin Grid (any
 *     "Sense" setting other than disabled; the driver selects falling edge at
 *     run time).  Otherwise CCLK_SetInterruptHandler() is not generated.
 *   - The PORTD interrupt vector itself is MCC's (pins.c); this driver only
 *     registers a callback with it.  Enable global interrupts after spcInit().
 *
 * Electrical notes:
 *   - CCLK and CDATA are inputs with the internal pull-up enabled (the
 *     Mitutoyo outputs are open-collector).  Set SPC_PULLUPS to 0 to disable.
 *   - CREQ is driven open-drain style: pulled low to request, released to
 *     high-impedance otherwise.  Define SPC_REQ_RELEASE_DRIVE_HIGH=1 only if
 *     you are sure the instrument's REQ input tolerates your VDD.
 *
 * Typical use:
 *
 *     spcInit();
 *     for (;;) {
 *         spc_reading_t r;
 *         spc_status_t s = spcRead(&r, millis());
 *         if (s == SPC_OK) {
 *             // value = r.counts / 10^r.decimals, in mm or inch (r.is_inch)
 *         }
 *         // SPC_PENDING: nothing yet, keep calling.
 *         // Error statuses: the next call starts a fresh request.
 *     }
 */
#ifndef SPC_H
#define SPC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Configuration (override with -D flags if needed)                           */
/* ------------------------------------------------------------------------- */

/* Enable internal pull-ups on CCLK and CDATA. */
#ifndef SPC_PULLUPS
#define SPC_PULLUPS 1
#endif

/* 0: release CREQ to high-impedance (default). 1: drive CREQ high. */
#ifndef SPC_REQ_RELEASE_DRIVE_HIGH
#define SPC_REQ_RELEASE_DRIVE_HIGH 0
#endif

/* Give up on a request that has not completed after this long. */
#ifndef SPC_TIMEOUT_MS
#define SPC_TIMEOUT_MS 250u
#endif

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

typedef enum {
    SPC_OK = 0,      /* *out holds a fresh, valid reading            */
    SPC_PENDING,     /* request in progress, call again later        */
    SPC_ERR_TIMEOUT, /* instrument did not finish a frame in time    */
    SPC_ERR_FORMAT   /* frame received but failed validation         */
} spc_status_t;

typedef struct {
    int32_t counts;    /* signed; value = counts / 10^decimals */
    uint8_t decimals;  /* digits after the decimal point       */
    bool is_inch;      /* false = mm, true = inch              */
} spc_reading_t;

/* Configure the pins and register the CCLK callback. Call once at start-up
 * (after MCC's SYSTEM_Initialize), then enable global interrupts. */
void spcInit(void);

/* Non-blocking. Poll it from the main loop.
 *   - Idle:        starts a new request and returns SPC_PENDING.
 *   - In progress: returns SPC_PENDING (or SPC_ERR_TIMEOUT after the timeout).
 *   - Finished:    returns SPC_OK with *out filled in, or SPC_ERR_FORMAT.
 * After OK/ERR_* the next call starts another request, so the call rate sets
 * the measurement rate.  now_ms is any free-running millisecond counter. */
spc_status_t spcRead(spc_reading_t *out, uint32_t now_ms);

/* Cancel a request in progress and release CREQ. */
void spcAbort(void);

/* Convenience conversion (uses software floating point on AVR). */
float spcToFloat(const spc_reading_t *r);

/* Formats the value as text, e.g. "-12.345" or "0.005" */
size_t formatSpcBuffer(char *buf, size_t size, const spc_reading_t *r);


#ifdef __cplusplus
}
#endif

#endif /* SPC_H */
