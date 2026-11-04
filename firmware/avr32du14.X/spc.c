/**
 * spc.c - Mitutoyo Digimatic (SPC) receiver for AVR DU, using the MCC
 * generated pin macros (CCLK_*, CDATA_*, CREQ_*) from pins.h.
 * See spc.h for the protocol description and usage.
 */
#include "mcc_generated_files/system/pins.h"

#include <stddef.h>
#include <util/atomic.h>

#include "spc.h"


#define SPC_FRAME_BITS 52u
#define SPC_NIBBLES 13u

enum {
    ST_IDLE = 0,    /* nothing in flight                                  */
    ST_WAIT_START,  /* CREQ asserted, waiting for the first CCLK edge     */
    ST_RECEIVING,   /* clocking bits in                                   */
    ST_DONE         /* 52 bits captured, waiting for the main loop        */
};

static volatile uint8_t s_state = ST_IDLE;
static volatile uint8_t s_bitcount;
static volatile uint8_t s_cur;                /* nibble being built  */
static volatile uint8_t s_nib[SPC_NIBBLES];   /* completed nibbles   */
static uint32_t s_t_start;

/* ------------------------------------------------------------------------- */
/* Pin helpers (thin wrappers over the pins.h macros)                         */
/* ------------------------------------------------------------------------- */
static inline void hwReqAssert(void)
{
    CREQ_SetLow();
    CREQ_SetDigitalOutput();
}

static inline void hwReqRelease(void)
{
#if SPC_REQ_RELEASE_DRIVE_HIGH
    CREQ_SetHigh();
#else
    CREQ_SetDigitalInput();
#endif
}

static inline void hwClkIrqEnable(void)
{
    CCLK_EnableInterruptForFallingEdge();
}

static inline void hwClkIrqDisable(void)
{
    CCLK_DisableInterruptOnChange();
}

/* ------------------------------------------------------------------------- */
/* Bit capture: runs once per falling CCLK edge                               */
/* ------------------------------------------------------------------------- */
static void onClkEdge(bool dataLevel)
{
    uint8_t st = s_state;
    if (st != ST_WAIT_START && st != ST_RECEIVING) {
        return; /* stray edge while idle/done */
    }

    uint8_t bc = s_bitcount;

    /* LSB first: shift right, new bit enters at bit 3. After four bits the
     * first bit has landed in bit 0 and the previous nibble has shifted out. */
    uint8_t cur = (uint8_t)((s_cur >> 1) | (dataLevel ? 0x08u : 0x00u));
    s_cur = cur;

    if ((bc & 3u) == 3u) {
        s_nib[bc >> 2] = cur;
        if (bc == 3u) {
            hwReqRelease(); /* first nibble received: CREQ may be dropped */
        }
    }

    bc++;
    s_bitcount = bc;

    if (bc >= SPC_FRAME_BITS) {
        hwClkIrqDisable();
        hwReqRelease(); /* no-op if already released */
        s_state = ST_DONE;
    } else if (st == ST_WAIT_START) {
        s_state = ST_RECEIVING;
    }
}

/* Registered with MCC's PORTD interrupt dispatcher; runs in interrupt context. */
static void onClkInterrupt(void)
{
    /* Sample CDATA first to keep latency from the clock edge minimal. */
    bool level = (CDATA_GetValue() != 0);
    onClkEdge(level);
}

/* ------------------------------------------------------------------------- */
/* Frame decoding                                                             */
/* ------------------------------------------------------------------------- */

/* Validate and decode 13 raw nibbles into a reading. */
static bool spcDecode(const uint8_t nib[SPC_NIBBLES], spc_reading_t *out)
{
    for (uint8_t i = 0; i < 4; i++) {
        if (nib[i] != 0x0F) {
            return false; /* header */
        }
    }
    if (nib[4] != 0x00 && nib[4] != 0x08) {
        return false; /* sign */
    }

    int32_t v = 0;
    for (uint8_t i = 5; i <= 10; i++) {
        if (nib[i] > 9) {
            return false; /* not a decimal digit */
        }
        v = v * 10 + nib[i];
    }
    if (nib[11] > 6) {
        return false; /* implausible decimal point position */
    }

    if (out != NULL) {
        out->counts = (nib[4] == 0x08) ? -v : v;
        out->decimals = nib[11];
        out->is_inch = (nib[12] != 0);
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */
void spcInit(void)
{
    /* CCLK and CDATA: inputs, optional pull-up, interrupt off until needed */
    CCLK_SetDigitalInput();
    CDATA_SetDigitalInput();
#if SPC_PULLUPS
    CCLK_SetPullUp();
    CDATA_SetPullUp();
#else
    CCLK_ResetPullUp();
    CDATA_ResetPullUp();
#endif
    CCLK_DisableInterruptOnChange();

    /* CREQ: released state (output latch low so asserting never glitches high) */
    CREQ_SetLow();
    hwReqRelease();

    s_state = ST_IDLE;
    CCLK_SetInterruptHandler(onClkInterrupt);
}

void spcAbort(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        hwClkIrqDisable();
        hwReqRelease();
        s_state = ST_IDLE;
    }
}

spc_status_t spcRead(spc_reading_t *out, uint32_t now_ms)
{
    uint8_t st = s_state;

    if (st == ST_IDLE) {
        s_t_start = now_ms;

        /* Arm the edge interrupt while the state is still IDLE: should the
         * pin-sense change raise a spurious flag, the callback ignores it. */
        hwClkIrqEnable();

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            s_bitcount = 0;
            s_cur = 0;
            s_state = ST_WAIT_START;
            hwReqAssert();
        }
        return SPC_PENDING;
    }

    if (st != ST_DONE) {
        if ((uint32_t)(now_ms - s_t_start) <= SPC_TIMEOUT_MS) {
            return SPC_PENDING;
        }
        bool timedOut = false;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            if (s_state != ST_DONE) { /* the frame may have just finished */
                hwClkIrqDisable();
                hwReqRelease();
                s_state = ST_IDLE;
                timedOut = true;
            }
        }
        if (timedOut) {
            return SPC_ERR_TIMEOUT;
        }
    }

    /* ST_DONE: the ISR no longer touches s_nib, so copy it out and re-arm. */
    uint8_t nib[SPC_NIBBLES];
    for (uint8_t i = 0; i < SPC_NIBBLES; i++) {
        nib[i] = s_nib[i];
    }
    s_state = ST_IDLE;

    return spcDecode(nib, out) ? SPC_OK : SPC_ERR_FORMAT;
}

float spcToFloat(const spc_reading_t *r)
{
    float v = (float)r->counts;
    for (uint8_t i = 0; i < r->decimals; i++) {
        v /= 10.0f;
    }
    return v;
}


/* Appends c at *pos if it fits (always leaving room for the terminator);
 * *pos keeps counting either way so the caller can detect truncation. */
static void bufPut(char *buf, size_t size, size_t *pos, char c)
{
    if (size > 0 && *pos < size - 1) {
        buf[*pos] = c;
    }
    (*pos)++;
}

/* Formats the value as text, e.g. "-12.345" or "0.005".
 * Like snprintf: writes at most size-1 characters plus a '\0' (when size > 0)
 * and returns the length the full string would have, excluding the '\0'.
 * A return value >= size means the output was truncated. */
size_t formatSpcBuffer(char *buf, size_t size, const spc_reading_t *r)
{
    size_t pos = 0;

    uint32_t mag = (r->counts < 0) ? (uint32_t)(-r->counts) : (uint32_t)r->counts;

    uint32_t div = 1;
    for (uint8_t i = 0; i < r->decimals; i++) {
        div *= 10;
    }

    uint32_t whole = mag / div;
    uint32_t frac = mag % div;

    if (r->counts < 0) {
        bufPut(buf, size, &pos, '-');
    }

    /* whole part: no leading zeros, but always at least one digit */
    uint32_t p = 1;
    while (whole / p >= 10) {
        p *= 10;
    }
    for (; p > 0; p /= 10) {
        bufPut(buf, size, &pos, (char)('0' + (whole / p) % 10));
    }

    /* fractional part: keeps leading zeros */
    if (r->decimals > 0) {
        bufPut(buf, size, &pos, '.');
        for (p = div / 10; p > 0; p /= 10) {
            bufPut(buf, size, &pos, (char)('0' + (frac / p) % 10));
        }
    }

    if (size > 0) {
        buf[(pos < size - 1) ? pos : size - 1] = '\0';
    }
    return pos;
}
