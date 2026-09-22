# BCA152 FreeRTOS Multisensor — ESP32 Room Monitor

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP--IDF-blue)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/framework-ESP--IDF%20v6.0-green)](https://docs.espressif.com/projects/esp-idf/)
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-orange)](https://www.freertos.org/)
[![Wokwi](https://img.shields.io/badge/simulator-Wokwi-purple)](https://wokwi.com/)
[![Tests](https://img.shields.io/badge/unit%20tests-13%2F13%20passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-lightgrey)]()

An ESP32 room monitor built with ESP-IDF and FreeRTOS, simulated in Wokwi. It reads temperature, humidity, ambient light, and motion, shows one measurement at a time on an SSD1306 OLED, and switches between ACTIVE and INACTIVE based on motion inactivity.

---

## Features

- DHT22 — temperature and humidity every 2 s
- LDR — ambient light as 0–100 %
- PIR input — motion detection with a 15 s inactivity timeout
- SSD1306 OLED — shows one measurement at a time
- KY-040 rotary encoder — cycles through Temp / Humidity / Light / Motion pages
- Buzzer — sounds when temperature leaves the 18–30 °C range
- ACTIVE/INACTIVE state machine — sleeps on inactivity, wakes on motion
- Thread-safe serial logging via mutex

---

## Architecture

![System Architecture](docs/images/system-architecture.png)

`main.cpp` handles hardware init and spawns the tasks. `rtos_objects.cpp` holds every queue, mutex, and event group in one place. Each module owns one task and one peripheral. Pure decision functions live in `logic.h` so they can be tested on the host.

![FreeRTOS Task Communication](docs/images/freertos-communication.png)

---

## Tasks

| Task | Priority | Period / Trigger | Blocks On |
|---|---|---|---|
| SensorTask | 2 | 2 s (`vTaskDelayUntil`) | Queue send |
| DisplayTask | 1 | Event-driven, 200 ms redraw | `xQueueReceive` (100 ms timeout) |
| InputTask | 3 | 5 ms polling | `vTaskDelay(1)` |
| MotionTask | 3 | 50 ms polling | Periodic delay |
| AlarmTask | 2 | Event-driven | `xQueueReceive` (block forever) |
| StateTask | 2 | 500 ms polling | `vTaskDelay` |

Priority reflects how urgent the response needs to be. Encoder and motion are felt by the user, so they get priority 3. Sensor, alarm, and state have relaxed timing budgets, so they get 2. The display tolerates tens of milliseconds invisibly, so it gets 1.

---

## Inter-Task Communication

| Primitive | Producer | Consumer | Purpose |
|---|---|---|---|
| `sensorQueue` (len 5) | SensorTask | DisplayTask | Decouple sampling from rendering |
| `alarmQueue` (len 5) | SensorTask | AlarmTask | Alarm logic on its own stack |
| `modeQueue` (len 1) | InputTask | DisplayTask | Latest-wins page selection |
| `serialMutex` | — | All `safe_print()` | Protect `vprintf` |
| `systemEvents` | MotionTask, StateTask, AlarmTask | Any observer | `EVENT_ACTIVE`, `EVENT_MOTION`, `EVENT_ALARM` |

Queues over shared globals: blocking receive, bounded buffer, no polling. Event group over task notification: multiple tasks can wait on independent bits. Mutex for serial: `vprintf` isn't thread-safe and two tasks will interleave bytes without it.

---

## State Machine

![State Machine](docs/images/state-machine.png)
15 s no motion
ACTIVE ──────────────────────────► INACTIVE
▲ │
└────────── motion detected ─────────┘

text

| State | OLED | Sensors | Encoder | Alarm |
|---|---|---|---|---|
| ACTIVE | Selected measurement | Full | Active | Active |
| INACTIVE | "SYSTEM INACTIVE" | Reduced | Inactive | Active |

Motion detection runs in both states so the system can wake on demand. `lastMotionTick` is reset on the INACTIVE → ACTIVE transition — without that, the next StateTask iteration sees a stale timeout and flips right back, producing visible flicker. Caught during development.

---

## Repository Layout
bca152-freertos-multisensor/
├── include/ # logic.h (pure), per-task interfaces, rtos_objects.h
├── src/ # main.cpp + one .cpp per module + rtos_objects.cpp
├── test/test_logic/ # 13 host-side unit tests
├── docs/ # laboratory report, diagrams, screenshots
├── platformio.ini
├── wokwi.toml
├── diagram.json
├── sdkconfig.defaults
└── README.md

text

---

## Build & Simulate

Requires VS Code, the PlatformIO IDE extension, the Wokwi extension, and Git. MinGW-w64 or MSYS2 is only needed for the native unit tests.

```bash
git clone https://github.com/jcreeredelosa-boop/bca152-freertos-multisensor.git
cd bca152-freertos-multisensor
pio run -e esp32dev
The first build downloads the ESP-IDF toolchain (~2 minutes). Later builds are much faster.

To simulate: open the project in VS Code, press Ctrl+Shift+P, then Wokwi: Start Simulator. Watch the Wokwi Terminal tab for boot logs.

Testing
Unit tests (run on the host PC):

bash
pio test -e native
Covers evaluateTemperature, nextDisplayMode, previousDisplayMode, and evaluateSystemState. All 13 cases pass.

Static analysis:

bash
pio check -e esp32dev
Result: 0 high, 0 medium, 14 low. The 14 are unusedFunction false positives (cppcheck can't trace calls made through xTaskCreate) plus one knownConditionTrueFalse from the simulated DHT22 driver.

Functional Verification
ID	Stimulus	Expected	Result
FT-01	Watch OLED for 10 s	Temp value updates	PASS
FT-02	Rotate to Humidity	Humidity updates	PASS
FT-03	Drag LDR slider	Light % changes	PASS
FT-04	Rotate encoder CW	Next page selected	PASS
FT-05	Rotate encoder CCW	Previous page, wraps	PASS
FT-06	Force T = 31.5 °C	Buzzer + HIGH log	PASS
FT-07	Return T = 25.4 °C	Buzzer stops	PASS
FT-08	Motion event	Motion detected, stays ACTIVE	PASS
FT-09	Idle 15 s	→ INACTIVE at 15.06 s	PASS
FT-10	Motion while INACTIVE	→ ACTIVE immediately	PASS
Screenshots in docs/screenshots/. Full record in docs/laboratory-report.pdf.

Notes & Limitations
Why vTaskDelayUntil over vTaskDelay. vTaskDelay(period) sleeps after the work finishes, so work time accumulates into every cycle — the real period becomes work + period. vTaskDelayUntil sleeps until an absolute deadline, so the sampling rate stays fixed no matter how long each read takes.

Simulated inputs. Wokwi's DHT22 doesn't respond reliably to ESP-IDF bit-bang drivers (both a custom implementation and esp-idf-lib/dht fail with a "phase B" init error), so sensor values are generated at the dht_read() boundary. The KY-040 rotary encoder UI in the ESP-IDF VS Code extension doesn't expose reliable CW/CCW controls, and Wokwi's photoresistor output impedance doesn't work with the ESP32 ADC input stage, so those reads are substituted too. Motion is a firmware-generated pattern (3 s motion, 27 s idle) for the same reason. Everything downstream — queues, event group, state transitions, rendering — is real. On physical hardware, the substituted functions would be replaced with real driver calls.

Serial race masked in Wokwi. The emulator serializes UART writes, so removing the mutex doesn't produce visible interleaving. The mutex is kept because vprintf isn't thread-safe on real hardware.

Legacy I2C driver. driver/i2c.h is deprecated in ESP-IDF v6.0 and slated for removal in v7.0. The warning is suppressed in sdkconfig.defaults. Migration to driver/i2c_master.h is planned.

Future work. Migrate to i2c_master.h, NVS-configurable temperature limits, SD card logging, MQTT telemetry, and validation on real DHT22 and PIR hardware.

References
ESP-IDF Programming Guide

FreeRTOS Reference Manual

Wokwi Documentation

PlatformIO ESP-IDF Guide

Barry, R. — Mastering the FreeRTOS Real Time Kernel

Course: BCA152 Microcontrollers, MSU-IIT, College of Computer Studies — Asst. Prof. Paul Rodolf P. Castor, M.Sc.

License
## How to use this

**1. Save it as `README.md` in your repo root.**

**2. The four images it references must exist**, or the image links will show broken icons:

| Path | What it is |
|---|---|
| `docs/images/system-architecture.png` | Layered diagram (main.cpp → rtos_objects.cpp → tasks → peripherals) |
| `docs/images/freertos-communication.png` | Task boxes with arrows for queues and event bits |
| `docs/images/state-machine.png` | ACTIVE/INACTIVE two-circle diagram |
| `docs/screenshots/wokwi-circuit.png` | Screenshot of the Wokwi canvas |

If you don't have them yet, either create them (draw.io for diagrams, Mermaid Live for the state machine) or temporarily delete the image lines. Broken images look worse than no images.

**3. Commit it:**

```bash
git add README.md
git commit -m "Rewrite README"
git push    