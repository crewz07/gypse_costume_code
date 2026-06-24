#include <Arduino.h>
#include "costume.h"

/* ============================================================
 *  SERIAL DIAGNOSTIC CONFIGURATION
 *  Serial uses the Uno hardware UART (D0/D1) claimed internally
 *  by the Serial library — those pins are not available as GPIO
 *  while Serial is active and are not defined in costume.h.
 * ============================================================ */
#define SERIAL_BAUD_RATE    115200U   /* USB serial baud rate                              */
#define SERIAL_REPORT_MS    1000U   /* State is printed over serial every this many ms   */

/* ============================================================
 *  STATE FUNCTION FORWARD DECLARATIONS
 *
 *  To add a new lighting state:
 *    1. Declare it here with STATE_FUNC(YourStateName);
 *    2. Define it in the "STATE FUNCTIONS" section below.
 *    3. Add state_YourStateName to state_table[].
 *  Nothing else needs to change.
 * ============================================================ */
STATE_FUNC(OFF);
STATE_FUNC(LB);
STATE_FUNC(RB);
STATE_FUNC(CR);
STATE_FUNC(ALL_SOLID);
STATE_FUNC(ALL_MARQUEE);

/* ============================================================
 *  STATE TABLE
 *  The machine steps forward through this array on each short
 *  press and resets to index 0 on a long press.
 *  NUM_STATES is always derived from the array — never hardcoded.
 *
 *  NOTE: ALL_SOLID (test mode) must always remain the LAST entry.
 *        Cycle states are every entry before it.
 * ============================================================ */
static StateFunc_t state_table[] = {
    state_OFF,        /* index 0 — all zones off                          */
    state_ALL_MARQUEE,/* index 1 — all zones marquee                      */
    state_LB,         /* index 2 — left-chest zone only                   */
    state_ALL_MARQUEE,/* index 3 — all zones marquee                      */
    state_RB,         /* index 4 — right-chest zone only                  */
    state_ALL_MARQUEE,/* index 5 — all zones marquee                      */
    state_CR,         /* index 6 — perineum zone only                     */
    state_ALL_MARQUEE,/* index 7 — all zones marquee (holding state)      */
    state_ALL_SOLID   /* index 8 — test mode only; enter with 4 s hold    */
};

#define NUM_STATES             ((uint8_t)(sizeof(state_table) / sizeof(state_table[0])))
#define LAST_CYCLE_STATE_INDEX ((uint8_t)(NUM_STATES - 2U))  /* Last normal cycle state   */
#define TEST_MODE_STATE_INDEX  ((uint8_t)(NUM_STATES - 1U))  /* ALL_SOLID test mode index */

static uint8_t state_index = 0;   /* Current position in state_table[] */

/* ============================================================
 *  BUTTON STATE VARIABLES
 * ============================================================ */
static uint8_t  raw_btn_state       = HIGH;  /* Most recent raw pin reading            */
static uint8_t  debounced_btn_state = HIGH;  /* Confirmed state after DEBOUNCE_MS stable */
static uint32_t last_raw_change_ms  = 0;     /* Timestamp of last raw reading change   */

static bool     btn_active            = false;  /* True while a press is in progress         */
static bool     long_press_fired      = false;  /* Guards against re-triggering at 2 s       */
static bool     very_long_press_fired = false;  /* Guards against re-triggering at 4 s       */
static uint32_t press_start_ms        = 0;      /* When the current press was first accepted */

/* ============================================================
 *  STATE TRANSITION HELPERS
 * ============================================================ */

/* Advance one step through the normal cycle; stop at the last cycle state */
static void on_short_press(void)
{
    if (state_index < LAST_CYCLE_STATE_INDEX) {
        state_index++;
    }
}

/* Jump back to the first state (OFF) */
static void on_long_press(void)
{
    state_index = 0;
}

/* Enter ALL_SOLID test mode (all zones fully on) */
static void on_very_long_press(void)
{
    state_index = TEST_MODE_STATE_INDEX;
}

/* ============================================================
 *  BUTTON TASK — call every loop() iteration
 *
 *  Debounce:        A raw pin change is only accepted after it has
 *                   been stable for DEBOUNCE_MS (50 ms). Any bounce
 *                   that settles within that window is ignored.
 *
 *  Short press:     Button press (falling edge after debounce) immediately
 *                   advances the state machine one step.
 *
 *  Long press:      Button held >= LONG_PRESS_MS (2000 ms) resets
 *                   the state machine to OFF while still held.
 *
 *  Very long press: Button held >= VERY_LONG_PRESS_MS (4000 ms)
 *                   enters ALL_SOLID test mode while still held.
 * ============================================================ */
static void button_task(void)
{
    uint8_t  current_raw = (uint8_t)digitalRead(PIN_BUTTON);
    uint32_t now         = millis();

    /* Restart the debounce timer whenever the raw reading changes */
    if (current_raw != raw_btn_state) {
        raw_btn_state      = current_raw;
        last_raw_change_ms = now;
    }

    /* Promote raw -> debounced once the reading has been
     * stable for DEBOUNCE_MS milliseconds                  */
    if ((raw_btn_state != debounced_btn_state) &&
        ((now - last_raw_change_ms) >= DEBOUNCE_MS)) {

        debounced_btn_state = raw_btn_state;

        /* Falling edge (active-low) — button was just pressed */
        if (debounced_btn_state == LOW) {
            btn_active            = true;
            long_press_fired      = false;
            very_long_press_fired = false;
            press_start_ms        = now;
            on_short_press();
        }

        /* Rising edge — button was just released */
        if (debounced_btn_state == HIGH) {
            btn_active = false;
        }
    }

    /* Long press (2 s) — fires once while the button is still held */
    if (btn_active && !long_press_fired &&
        ((now - press_start_ms) >= LONG_PRESS_MS)) {
        long_press_fired = true;
        on_long_press();
    }

    /* Very long press (4 s) — fires once while the button is still held */
    if (btn_active && !very_long_press_fired &&
        ((now - press_start_ms) >= VERY_LONG_PRESS_MS)) {
        very_long_press_fired = true;
        on_very_long_press();
    }
}

/* ============================================================
 *  SERIAL STATE REPORTER
 *  Prints the current state name, active pins, and behavior
 *  once per SERIAL_REPORT_MS.  Uses function pointer comparison
 *  so no indices are hardcoded here.
 *  F() keeps string literals in flash, saving RAM.
 * ============================================================ */
static void print_state(void)
{
    StateFunc_t current = state_table[state_index];

    Serial.print(millis());
    Serial.print(F("ms | "));

    if (current == state_OFF) {
        Serial.println(F("OFF — all LED pins LOW"));
    }
    else if (current == state_LB) {
        Serial.print(F("LB — pin "));
        Serial.print(PIN_LB_A);
        Serial.print(F("=H  pin "));
        Serial.print(PIN_LB_B);
        Serial.println(F("=H  |  all others LOW"));
    }
    else if (current == state_RB) {
        Serial.print(F("RB — pin "));
        Serial.print(PIN_RB_A);
        Serial.print(F("=H  pin "));
        Serial.print(PIN_RB_B);
        Serial.println(F("=H  |  all others LOW"));
    }
    else if (current == state_CR) {
        Serial.print(F("CR — pin "));
        Serial.print(PIN_CR_A);
        Serial.print(F("=H  pin "));
        Serial.print(PIN_CR_B);
        Serial.println(F("=H  |  all others LOW"));
    }
    else if (current == state_ALL_MARQUEE) {
        if (state_index == LAST_CYCLE_STATE_INDEX) {
            Serial.print(F("ALL_MARQUEE [HOLD]"));
        } else {
            Serial.print(F("ALL_MARQUEE"));
        }
        Serial.println(F(" — all zones A<->B @ 250ms [MARQUEE]"));
        Serial.print(F("      LB("));
        Serial.print(PIN_LB_A);
        Serial.print(F("<->"));
        Serial.print(PIN_LB_B);
        Serial.print(F(")  RB("));
        Serial.print(PIN_RB_A);
        Serial.print(F("<->"));
        Serial.print(PIN_RB_B);
        Serial.print(F(")  CR("));
        Serial.print(PIN_CR_A);
        Serial.print(F("<->"));
        Serial.print(PIN_CR_B);
        Serial.print(F(")  SHLD("));
        Serial.print(PIN_SHLD_A);
        Serial.print(F("<->"));
        Serial.print(PIN_SHLD_B);
        Serial.println(F(")"));
    }
    else if (current == state_ALL_SOLID) {
        Serial.println(F("ALL_SOLID [TEST MODE] — all LED pins HIGH"));
    }
    else {
        Serial.println(F("UNKNOWN STATE"));
    }
}

/* ============================================================
 *  STATE FUNCTIONS
 *  Each function sets the LED outputs for its lighting state.
 *  These are called continuously from loop(), so marquee uses
 *  a non-blocking timer rather than delay().
 * ============================================================ */

STATE_FUNC(OFF)
{
    ALL_ZONES_OFF();
}

STATE_FUNC(LB)
{
    ONE_ZONE_ON(ZONE_LB);
}

STATE_FUNC(RB)
{
    ONE_ZONE_ON(ZONE_RB);
}

STATE_FUNC(CR)
{
    ONE_ZONE_ON(ZONE_CR);
}

STATE_FUNC(ALL_SOLID)
{
    ALL_ZONES_SOLID();
}

STATE_FUNC(ALL_MARQUEE)
{
    static uint32_t last_toggle_ms = 0;
    static bool     side_a_active  = true;

    if ((millis() - last_toggle_ms) >= MARQUEE_PERIOD_MS) {
        last_toggle_ms = millis();
        if (side_a_active) {
            ALL_ZONES_SET_A();
        } else {
            ALL_ZONES_SET_B();
        }
        side_a_active = !side_a_active;
    }
}

/* ============================================================
 *  ARDUINO ENTRY POINTS
 * ============================================================ */

void setup(void)
{
    uint8_t i;

    /* Configure all zone LED pins as outputs */
    for (i = 0; i < NUM_ZONES; i++) {
        pinMode(ZONES[i].pinA, OUTPUT);
        pinMode(ZONES[i].pinB, OUTPUT);
    }

    /* Button is active-low — enable the internal pull-up resistor */
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    /* Start USB serial reporting */
    Serial.begin(SERIAL_BAUD_RATE);

    /* Apply the initial state (OFF) */
    state_table[state_index]();
}

void loop(void)
{
    static uint32_t last_report_ms = 0;
    uint32_t        now            = millis();

    button_task();
    state_table[state_index]();

    /* Print current state over USB serial once per SERIAL_REPORT_MS */
    if ((now - last_report_ms) >= SERIAL_REPORT_MS) {
        last_report_ms = now;
        print_state();
    }
}
