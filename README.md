# Webbi — An ESP32 Web-Traffic Counter on a Multiplexed 7-Segment Display

Webbi is a small standalone device that shows, in glowing red digits, how many requests
your website has received. A microcontroller on your desk asks a server "what's the count
right now?" every few seconds and lights up the answer on six 7-segment digits.

```
 Server (Colossus / Cloudflare)
        │  HTTP GET  → { "requests": 52415, ... }
        ▼
     ESP32  ── parses JSON ──► display buffer
        │
        ▼
 Multiplexed 7-segment display  →  5 2 4 1 5
```

This README teaches the project **from zero**. By the end you'll understand the physics
(voltage, current, LEDs), the electronics (transistors, common-anode displays,
multiplexing), the firmware (non-blocking refresh, WiFi, JSON), and the exact terminal
commands to build and flash it. You can replicate Webbi even if you've never touched a
breadboard.

> Author: Ryann Mack · Built at the CyPhy Life lab

---

## Table of contents

1. [The physics you actually need](#1-the-physics-you-actually-need)
2. [What a 7-segment display is](#2-what-a-7-segment-display-is)
3. [Why the ESP32 can't drive the display by itself](#3-why-the-esp32-cant-drive-the-display-by-itself)
4. [Transistors: the electronic switch](#4-transistors-the-electronic-switch)
5. [Multiplexing: 6 digits without 56 wires](#5-multiplexing-6-digits-without-56-wires)
6. [The full circuit design](#6-the-full-circuit-design)
7. [Bill of materials](#7-bill-of-materials)
8. [Wiring, step by step](#8-wiring-step-by-step)
9. [The software architecture](#9-the-software-architecture)
10. [Getting it running — terminal commands](#10-getting-it-running--terminal-commands)
11. [The backend (server side)](#11-the-backend-server-side)
12. [Troubleshooting](#12-troubleshooting)
13. [Repository layout](#13-repository-layout)

---

## 1. The physics you actually need

Electricity is easiest to picture as **water in pipes**. Three quantities matter, and one
equation ties them together.

| Symbol | Name | Water analogy | Unit |
|--------|------|---------------|------|
| **V** | Voltage | Pressure difference between two points | Volts (V) |
| **I** | Current | How much water flows per second | Amps (A) |
| **R** | Resistance | How narrow the pipe is | Ohms (Ω) |

**Ohm's law** — the single most important equation in this project:

```
V = I × R        I = V / R        R = V / I
```

Two non-negotiable facts that explain almost every bug you'll hit:

1. **Current only flows in a complete loop.** Power → component → back to power.
   If the loop is broken, nothing happens. The return side of that loop is called
   **ground (GND)**, which we define as 0 V — the reference everything is measured against.
2. **Current only flows when the two ends of a component are at *different* voltages.**
   If both legs of an LED sit at the same voltage, there is no "pressure difference," so
   no current, so no light — even though power is technically connected.

### LEDs are one-way valves that need a minimum push

An LED (Light-Emitting Diode) passes current in only one direction (anode → cathode) and
only above a **forward voltage** `Vf`. Below `Vf` it stays dark; above it, it conducts
*hard* and will destroy itself if you don't limit the current. That's why **every LED
needs a series resistor**.

Webbi's display segments are special: each segment is **two red LEDs in series**, so each
segment needs about **4 V** to light. Hold that number — it drives the whole power design.

### Sizing the resistor (a real calculation)

Supply is 5 V, the segment "uses" ~4 V, and we want a safe, bright current of ~10 mA:

```
Voltage left for the resistor = 5 V − 4 V ≈ 1 V
R = V / I = 1 V / 0.010 A = 100 Ω
```

So **~100 Ω per segment**. Smaller resistor → more current → brighter (but hotter and
riskier); larger → dimmer. (In practice the switching transistors drop another ~0.2 V, so
68–100 Ω is the bright-but-safe range.)

---

## 2. What a 7-segment display is

A 7-segment display is just **7 LEDs arranged in a figure-8**, labelled A–G, plus an
optional decimal point (DP):

```
   AAA
  F   B
  F   B
   GGG
  E   C
  E   C
   DDD
```

To show a digit you light the right subset. For example **"1"** = segments B and C;
**"7"** = A, B, C; **"8"** = all of them.

### Common anode vs. common cathode — this matters

All 7 LEDs share one common pin. There are two wiring conventions:

- **Common cathode:** all the LED *negatives* are tied together to GND. A segment lights
  when you drive its pin **HIGH** (push current in).
- **Common anode (what Webbi uses):** all the LED *positives* are tied together to +V. A
  segment lights when you pull its individual pin **LOW** (give the current a path out to
  ground).

> Webbi uses **common-anode** displays (model TOS-1160BE-B-21P). Common pin → +5 V;
> a segment turns on when its cathode line is connected toward ground.

The current path for one lit segment is always:

```
+5 V → common anode → LED segment → resistor → (switch to ground) → GND
```

---

## 3. Why the ESP32 can't drive the display by itself

The **ESP32** is a dual-core, WiFi-enabled microcontroller — think "tiny computer with
pins." Its GPIO (General-Purpose Input/Output) pins can be set HIGH or LOW in software:

```
digitalWrite(pin, HIGH);  // pin ≈ 3.3 V
digitalWrite(pin, LOW);   // pin ≈ 0 V (ground)
```

But there are two hard limits:

1. **Voltage:** ESP32 pins output only **3.3 V**. Our segments need **~4 V**. 3.3 V < 4 V,
   so the ESP32 literally cannot push enough pressure to light a segment directly.
2. **Current:** an ESP32 pin can safely source/sink only ~10–20 mA. Six digits' worth of
   segments would draw far more than the chip can survive.

**Conclusion:** the ESP32 must not *power* the display. It should only *control switches*.
The real energy comes from a separate 5 V supply; the ESP32 just opens and closes valves.

> Mental model: ESP32 = the finger, transistor = the faucet, 5 V supply = the water tank,
> display = the hose. Your finger doesn't push the water — it opens the faucet.

A critical rule that follows: **all grounds must be shared.** The ESP32 GND, the 5 V supply
GND, and the display side all connect to one common ground node, or none of the voltages
mean anything relative to each other.

---

## 4. Transistors: the electronic switch

A transistor is an **electrically controlled switch**: a small signal on one terminal
(the **base**) controls a much larger current through the other two (**collector** and
**emitter**). There are two flavours we use.

### NPN (e.g. 2N2222) — a switch to *ground* (low-side)

- **Emitter → GND, Collector → load, Base → control.**
- Turns **ON when the base is driven HIGH** (above ~0.7 V relative to the emitter).
- When on, it connects the load down to ground.
- Used for the **segments**: "ESP32, please connect this segment line to ground."

### PNP (e.g. 2N3906) — a switch to *+5 V* (high-side)

- **Emitter → +5 V, Collector → load, Base → control.**
- Turns **ON when the base is pulled LOW** (below the emitter by ~0.7 V).
- When on, it delivers +5 V to the load.
- Used for the **digit enable**: "ESP32, please connect this digit's common anode to +5 V."

### Why a PNP digit needs a helper NPN

A PNP on a 5 V rail wants its base pulled toward 0 V to turn on — but a 3.3 V GPIO can't
cleanly do that against a 5 V emitter. So we add a tiny **NPN pre-driver** as a level
shifter for each digit:

```
        +5 V
         │
        [10kΩ]  ← pull-up keeps PNP OFF by default
         │
 GPIO ──[1kΩ]── NPN base        node X ── PNP base ──[2.2kΩ in series]
                NPN emitter → GND   │
                NPN collector ──────┘
                                    PNP emitter → +5 V
                                    PNP collector → digit common anode
```

Logic of the whole digit stage:

```
GPIO HIGH → NPN ON → node X pulled LOW → PNP ON  → digit gets +5 V  (digit ON)
GPIO LOW  → NPN OFF → 10kΩ pulls node X HIGH → PNP OFF → digit dark  (digit OFF)
```

### The polarity that the firmware depends on

Because of the driver topology above, **both segments and digits are active-HIGH**
(idle LOW):

| Line | Driver | "ON" GPIO state |
|------|--------|-----------------|
| Segment A–G | NPN low-side | **HIGH** |
| Digit D1–D6 | PNP + NPN pre-driver | **HIGH** |

> ⚠️ If you build a *direct* PNP digit switch (no NPN pre-driver), the digit polarity flips
> to active-LOW. The firmware exposes a one-line `DIGIT_ON` / `DIGIT_OFF` knob for exactly
> this case (see `display.cpp`).

---

## 5. Multiplexing: 6 digits without 56 wires

Six digits × 8 segments = 48 LED lines, plus commons. Wiring each separately is absurd.
**Multiplexing** is the trick:

- **Share the 7 segment lines across all digits.** Segment A of every digit is the same
  wire; same for B, C, … G.
- **Give each digit its own enable line** (the common-anode control).
- **Light one digit at a time, very fast.** Enable digit 1, set its segment pattern, move
  to digit 2, and so on. Cycle through all six hundreds of times per second.

Your eye can't follow switching faster than ~60 Hz, so it blends the rapid flashes into a
steady image — the same illusion as blinking Christmas lights or a spinning bicycle wheel.

This cuts the wiring to **7 segment lines + 6 digit lines = 13 control lines** instead of ~48.

Two things to get right or it looks bad:

- **Ghosting** = faint leftover glow on a digit that should be off, because segments changed
  while a digit was still powered. Fix: **blank all digits first, then set segments, then
  enable the one digit** (the firmware does this).
- **Refresh rate** = full cycles per second. Webbi holds each digit ~2 ms, so 6 digits ×
  2 ms = 12 ms per frame ≈ **83 Hz** — flicker-free.

Because each digit is only on 1/6 of the time, multiplexed displays look dimmer; that's
normal and is why you can push the segment current a little higher than for a static LED.

---

## 6. The full circuit design

Putting it together, one segment lighting up looks like this:

```
                 +5 V
                  │
         [PNP digit switch]  ◄── enabled by ESP32 digit pin (via NPN pre-driver)
                  │
            digit common anode
                  │
     ┌────────────┴────────────┐
     A   B   C   D   E   F   G   (segment LEDs inside the display)
     │   │   │   │   │   │   │
   [100Ω resistor per segment]
     │   │   │   │   │   │   │
  [NPN] ... one per segment ...  ◄── base driven by ESP32 segment pin (via 1kΩ)
     │   │   │   │   │   │   │
     └────────────┬────────────┘
                  │
                 GND  (shared with ESP32 and 5 V supply)
```

A segment lights **only when both** are true: its digit's PNP is on (anode at +5 V) **and**
its segment's NPN is on (cathode pulled to ground). The PNP chooses *which digit*; the NPNs
choose *which segments*. Both together = light.

---

## 7. Bill of materials

| Qty | Part | Purpose |
|-----|------|---------|
| 1 | ESP32-WROOM dev board (ESP32-D0WD-V3) | The brain + WiFi |
| 6 | Common-anode 7-segment displays (TOS-1160BE-B-21P) | The digits |
| 6 | PNP transistor 2N3906 | High-side digit switches |
| ~13 | NPN transistor 2N2222 | 7 segment switches + 6 digit pre-drivers |
| 7 | 100 Ω resistor | Segment current limiting |
| ~13 | 1 kΩ resistor | NPN base resistors |
| 6 | 2.2 kΩ resistor | PNP base resistors |
| 6 | 10 kΩ resistor | PNP base pull-ups (keep digits off by default) |
| 1 | 5 V regulated supply (≥1 A) | Powers the display side |
| — | Breadboard, jumper wires, USB cable | Assembly + flashing |

> Start with **4 displays** to validate, then expand to 6. The firmware scales by changing
> a single constant (`NUM_DIGITS`).

---

## 8. Wiring, step by step

**Pin transistor reminder (flat side facing you):** for the 2N3906/2N2222 in this build the
legs are **Left = Collector, Middle = Base, Right = Emitter**. *Always confirm against your
specific part's datasheet — pinouts differ between manufacturers.*

### GPIO map

**Segments (A–G):**

| Segment | GPIO |
|---------|------|
| A | 18 |
| B | 19 |
| C | 21 |
| D | 22 |
| E | 23 |
| F | 25 |
| G | 26 |

**Digits (left → right, most significant first):**

| Digit | GPIO |
|-------|------|
| D1 | 32 |
| D2 | 33 |
| D3 | 27 |
| D4 | 13 |
| D5 *(future)* | 4 |
| D6 *(future)* | 17 |

> Avoid the boot-sensitive pins (GPIO0, 2, 12, 15), the input-only pins (GPIO34, 35), and
> the internal-flash pins (GPIO6–11). The map above already does.

### Order of assembly (build the loop, then test one piece at a time)

1. **Power rails first.** 5 V supply `+` → breadboard `+` rail; supply GND → `−` rail;
   **ESP32 GND → same `−` rail.** This shared ground is the #1 thing to get right.
2. **One digit switch (PNP + NPN pre-driver).** Wire the stage from
   [section 4](#why-a-pnp-digit-needs-a-helper-npn): PNP emitter → +5 V, collector → that
   display's common anode; NPN pre-driver from the digit GPIO. Verify the common anode
   reads ~5 V when the GPIO is HIGH.
3. **One segment switch (NPN low-side).** Segment line → 100 Ω → NPN collector; NPN
   emitter → GND; NPN base → 1 kΩ → segment GPIO. Verify the segment lights when both its
   digit and its segment GPIO are HIGH.
4. **Repeat** for all 7 segments and all digits. Segment lines are **shared** across every
   digit (that's the multiplexing).
5. **Sanity-check with a multimeter** in continuity mode (power off) before powering up.

---

## 9. The software architecture

The firmware is split so networking never blocks the display:

```
webbi_display.ino   ← setup + loop (orchestration, timing)
display.cpp / .h     ← multiplexing, digit buffer, leading-zero suppression
api_client.cpp / .h  ← HTTP GET + JSON parse  → currentVisitors
wifi_manager.cpp/.h  ← connects to WiFi with a timeout
wifi.h               ← your SSID/password (LOCAL ONLY, git-ignored)
wifi.example.h       ← safe template that IS committed
```

Key design decisions and *why*:

- **The display refresh is non-blocking.** `refreshDisplay()` lights one digit per call and
  returns immediately; it's called every iteration of `loop()`. There are **no `delay()`
  calls** in the main path, so multiplexing stays smooth.
- **Networking is on a timer.** Every ~5 s, `loop()` does one HTTP GET. The request is
  synchronous but short (bounded by a ~3 s timeout), so at worst you see a faint blink, not
  a freeze.
- **The display only ever reads from a buffer.** `setNumber()` splits the value into digits
  and pre-computes leading-zero suppression; `refreshDisplay()` just renders the buffer.
  This cleanly separates "what to show" from "how to show it."
- **Polarity is active-HIGH** for both segments and digits, matching the driver topology,
  and is exposed as named constants so it's trivial to flip if your hardware differs.
- **Scaling is one constant.** `NUM_DIGITS = 4` now; set it to `6` after soldering. If the
  number has more digits than are wired, the low digits roll into view
  (e.g. `52415` on 4 digits shows `2415`).

The server returns several fields; for validation Webbi uses only `requests` because it's a
cumulative, always-increasing number:

```json
{ "unique_visitors": 5710, "page_views": 6130, "requests": 52415,
  "period": "last_30_days", "updated_at": "2026-05-28T16:27:44Z" }
```

---

## 10. Getting it running — terminal commands

Webbi is developed under **WSL (Ubuntu on Windows)**, flashing the ESP32 over USB using
`usbipd` (to pass the USB device into WSL) and `arduino-cli`.

### 10.1 One-time toolchain setup (inside WSL/Ubuntu)

```bash
# Install arduino-cli if you don't have it
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Tell arduino-cli where to find the ESP32 boards
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

# Install the one external library
arduino-cli lib install "ArduinoJson"

# Let your user access serial ports (log out / back in after this)
sudo usermod -a -G dialout $USER
```

Optional: a reproducible toolchain environment via conda (`environment.yml` is in the repo):

```bash
conda env create -f environment.yml
conda activate webbi_env
```

### 10.2 Add your WiFi + API details

```bash
cp arduino/webbi_display/wifi.example.h arduino/webbi_display/wifi.h
# then edit wifi.h and fill in your real 2.4 GHz SSID + password
```

> The ESP32-WROOM is **2.4 GHz only** — it will not see a 5 GHz network. Capitalization and
> password must match exactly. `wifi.h` is git-ignored so your password never gets committed.

The API URL lives in `api_client.cpp`:

```cpp
const char* apiURL = "http://cyphylife.com/api/visitors";
```

Use plain **HTTP** (not HTTPS) while validating to avoid TLS/certificate complications.

### 10.3 Pass the ESP32 into WSL (PowerShell **as Administrator**)

```powershell
usbipd list                       # find the CP210x / USB-UART device's BUSID (e.g. 1-4)
usbipd bind --busid 1-4           # one-time, requires admin
usbipd attach --wsl --busid 1-4   # share it with WSL (repeat each plug-in, or use --auto-attach)
```

### 10.4 Compile, flash, and watch (inside WSL)

The easiest path is the helper script, which auto-detects the bus id, compiles, and uploads:

```bash
./upload.sh
```

Or do it manually:

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 arduino/webbi_display

# Upload (port is usually /dev/ttyUSB0)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 arduino/webbi_display

# Watch the serial output to confirm WiFi + the fetched count
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Expected serial output:

```
Connecting to WiFi...
.....
WiFi connected!
IP Address: 192.168.x.x
requests = 52415
```

If a port is stuck/busy:

```bash
lsof /dev/ttyUSB0      # find the process id (PID) holding it
kill -9 <PID>
```

---

## 11. The backend (server side)

The ESP32 stays simple by talking to a small backend that holds the secrets and exposes one
tiny endpoint. From a clean `backend/` folder:

```bash
cd backend
npm init -y
npm install express axios dotenv
```

Create `backend/.env` (git-ignored — never commit it):

```
CLOUDFLARE_API_TOKEN=your_token_here
CLOUDFLARE_ZONE_ID=your_zone_id_here
PORT=3000
```

Run and test:

```bash
node server.js
# in another terminal:
curl http://localhost:3000/api/visitors
# → { "visitors": 52415 }
```

The backend fetches analytics from the source (e.g. Cloudflare), extracts the count, and
returns a minimal JSON object. The ESP32 only ever sees that small, safe payload.

---

## 12. Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Only one digit lights, shows a dim `0` | Count is 0 *and* leading-zero suppression hides the rest; usually means networking returned nothing | Check WiFi + API on the serial monitor; the value should be non-zero |
| Nothing lights at all | Shared ground missing, or wrong polarity | Verify ESP32/5 V/display grounds are one node; check `DIGIT_ON`/`SEG_ON` |
| A whole digit or segment is inverted | Your driver polarity differs from the firmware assumption | Flip the matching `*_ON`/`*_OFF` constant in `display.cpp` |
| Faint "ghost" segments | Segments changed while a digit was still powered | Already handled (blank → set → enable); raise refresh rate if needed |
| Everything is dim | Each digit only on 1/N of the time | Lower segment resistors toward 68–100 Ω; keep refresh fast |
| ESP32 not found in WSL | usbipd bus id changed on re-plug | Re-run `usbipd attach`, or use `./upload.sh` (auto-detects) |
| WiFi never connects | 5 GHz network, wrong password, or special characters | Use a 2.4 GHz SSID; try a phone hotspot to isolate the issue |

The golden debugging habit: **change one thing at a time, and trace the loop.** For any dead
segment ask — is there a complete path from +5 V through the LED to ground, and is each
switch in that path actually on?

---

## 13. Repository layout

```
webbi/
├── arduino/
│   └── webbi_display/
│       ├── webbi_display.ino    # setup + non-blocking loop
│       ├── display.cpp / .h     # multiplexing + digit rendering
│       ├── api_client.cpp / .h  # HTTP GET + JSON → count
│       ├── wifi_manager.cpp/.h  # WiFi connect with timeout
│       ├── wifi.h               # YOUR secrets (git-ignored)
│       └── wifi.example.h       # safe template (committed)
├── backend/
│   ├── server.js                # API server
│   ├── routes/visitors.js       # fetches + returns the count
│   └── .env                     # secrets (git-ignored)
├── upload.sh                    # auto-detect + compile + flash (WSL)
├── environment.yml              # optional conda toolchain
├── .gitignore
└── README.md                    # you are here
```

---
