#ifndef COSTUME_H
#define COSTUME_H

#include <Arduino.h>
#include <stdint.h>

/* ============================================================
 *  MACRO STYLE NOTE
 *  All multi-statement macros in this file are wrapped in
 *  do { ... } while (0).  This makes each macro a single
 *  statement so it behaves safely in any context — including
 *  an if/else written without braces — regardless of who calls
 *  it.  As a complementary rule, all if/else blocks in this
 *  project should also use braces { }.  Both practices together
 *  eliminate an entire class of silent expansion bugs.
 * ============================================================ */

/* ============================================================
 *  PIN ASSIGNMENTS  —  update these to match your wiring
 * ============================================================ */
#define PIN_BUTTON      2 /* - internal pull-up, active LOW */
#define PIN_LB_A        3
#define PIN_LB_B        4
#define PIN_RB_A        5
#define PIN_RB_B        6
#define PIN_CR_A        7
#define PIN_CR_B        8
#define PIN_SHLD_A      9
#define PIN_SHLD_B      10

/* ============================================================
 *  TIMING CONSTANTS
 * ============================================================ */
#define DEBOUNCE_MS         50U     /* Reject button changes faster than this (ms)          */
#define SHORT_PRESS_MS      350U    /* Minimum hold time to count as a short press (ms)     */
#define LONG_PRESS_MS       2000U   /* Hold time to reset to OFF (ms)                        */
#define VERY_LONG_PRESS_MS  4000U   /* Hold time to enter ALL_SOLID test mode (ms)           */
#define MARQUEE_PERIOD_MS   250U    /* Each LED set stays on this long during marquee (ms)   */

/* ============================================================
 *  ZONE TABLE
 *  Each zone has two independently-controllable LED sets (A, B).
 *  Zones are identified by their index in ZONES[].
 * ============================================================ */
typedef struct {
    uint8_t pinA;   /* LED set A */
    uint8_t pinB;   /* LED set B */
} Zone_t;

/* Zone identifiers — these are indices into ZONES[] below */
typedef enum {
    ZONE_LB   = 0,
    ZONE_RB   = 1,
    ZONE_CR   = 2,
    ZONE_SHLD = 3
} ZoneIndex_t;

/* Zone pin mapping — order must match ZoneIndex_t */
static const Zone_t ZONES[] = {
    { PIN_LB_A,   PIN_LB_B   },   /* ZONE_LB   */
    { PIN_RB_A,   PIN_RB_B   },   /* ZONE_RB   */
    { PIN_CR_A,   PIN_CR_B   },   /* ZONE_CR   */
    { PIN_SHLD_A, PIN_SHLD_B }    /* ZONE_SHLD */
};

#define NUM_ZONES   ((uint8_t)(sizeof(ZONES) / sizeof(ZONES[0])))

/* ============================================================
 *  LED CONTROL MACROS
 * ============================================================ */

/* Turn both LED sets of one zone ON */
#define ZONE_BOTH_ON(z)     do { digitalWrite((z).pinA, HIGH); \
                                 digitalWrite((z).pinB, HIGH); } while (0)

/* Turn both LED sets of one zone OFF */
#define ZONE_BOTH_OFF(z)    do { digitalWrite((z).pinA, LOW);  \
                                 digitalWrite((z).pinB, LOW);  } while (0)

/* Marquee side A — set A on, set B off */
#define ZONE_SET_A(z)       do { digitalWrite((z).pinA, HIGH); \
                                 digitalWrite((z).pinB, LOW);  } while (0)

/* Marquee side B — set A off, set B on */
#define ZONE_SET_B(z)       do { digitalWrite((z).pinA, LOW);  \
                                 digitalWrite((z).pinB, HIGH); } while (0)

/* Turn every zone off */
#define ALL_ZONES_OFF() \
    do { uint8_t _i; \
         for (_i = 0; _i < NUM_ZONES; _i++) { ZONE_BOTH_OFF(ZONES[_i]); } \
    } while (0)

/* Turn every zone fully on */
#define ALL_ZONES_SOLID() \
    do { uint8_t _i; \
         for (_i = 0; _i < NUM_ZONES; _i++) { ZONE_BOTH_ON(ZONES[_i]); } \
    } while (0)

/* Marquee helpers — set all zones to side A or side B simultaneously */
#define ALL_ZONES_SET_A() \
    do { uint8_t _i; \
         for (_i = 0; _i < NUM_ZONES; _i++) { ZONE_SET_A(ZONES[_i]); } \
    } while (0)

#define ALL_ZONES_SET_B() \
    do { uint8_t _i; \
         for (_i = 0; _i < NUM_ZONES; _i++) { ZONE_SET_B(ZONES[_i]); } \
    } while (0)

/* Turn one zone ON, all other zones OFF.
 * The target zone is set to ON directly — it is never passed through
 * an ALL_ZONES_OFF phase — so it stays lit without any flicker. */
#define ONE_ZONE_ON(zone_idx) \
    do { uint8_t _i; \
         for (_i = 0; _i < NUM_ZONES; _i++) { \
             if (_i == (uint8_t)(zone_idx)) { ZONE_BOTH_ON(ZONES[_i]);  } \
             else                           { ZONE_BOTH_OFF(ZONES[_i]); } \
         } \
    } while (0)

/* ============================================================
 *  STATE FUNCTION MACRO
 *  STATE_FUNC(LB)  expands to  static void state_LB(void)
 *  Use for both declarations (add ;) and definitions (add { }).
 * ============================================================ */
#define STATE_FUNC(name)    static void state_##name(void)

/* ============================================================
 *  STATE MACHINE TYPES
 * ============================================================ */
typedef void (*StateFunc_t)(void);

#endif /* COSTUME_H */
