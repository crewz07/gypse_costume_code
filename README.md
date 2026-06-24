# Gypse Costume LED Controller

Arduino Uno firmware that drives a multi-zone LED lighting system on a wearable costume. A single button cycles through lighting modes; the state machine in `state_machine.md` defines every transition.

---

## Hardware

| Component | Detail |
|---|---|
| MCU | Arduino Uno |
| Button | D2 — active-low, internal pull-up |
| LED Zones | 4 zones × 2 independently-switched LED sets (A/B) |

**Zone → Pin mapping**

| Zone | Pin A | Pin B | Description |
|---|---|---|---|
| LB | D3 | D4 | Left chest |
| RB | D5 | D6 | Right chest |
| CR | D7 | D8 | Center / perineum |
| SHLD | D9 | D10 | Shoulders & Hips |

---

## How It Works

The firmware runs a table-driven state machine. Each entry in `state_table[]` is a function pointer; `loop()` calls the active state function on every iteration.

**Button gestures**

| Gesture | Threshold | Action |
|---|---|---|
| Short press | < 2 s | Advance one step in the cycle |
| Long press | ≥ 2 s | Reset to `OFF` (state 0) |
| Very long press | ≥ 4 s | Enter `ALL_SOLID` test mode |

Presses shorter than 50 ms (debounce) or 350 ms (minimum valid press) are discarded.

**Lighting cycle** (short-press progression)

```
OFF → ALL_MARQUEE → LB → ALL_MARQUEE → RB → ALL_MARQUEE → CR → ALL_MARQUEE [HOLD]
```

- **OFF** — all zones dark  
- **LB / RB / CR** — single zone on, all others off  
- **ALL_MARQUEE** — all zones alternate A↔B sets every 250 ms  
- **ALL_SOLID** — all zones fully on (test mode only; not reachable via short press)

> See [`state_machine.md`](state_machine.md) for the full PlantUML state diagram including all transitions.

---

## Serial Diagnostics

Connect at **115200 baud**. The active state, pin levels, and timestamp are reported every 1 s over USB serial.

---

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload to connected Uno
pio run --target upload

# Open serial monitor
pio device monitor --baud 115200
```

---

## Adding a New State

1. Declare it in `main.cpp`: `STATE_FUNC(YourState);`
2. Define the function body in the *STATE FUNCTIONS* section.
3. Insert `state_YourState` at the desired position in `state_table[]`.

`NUM_STATES` is derived automatically — no other changes needed.
