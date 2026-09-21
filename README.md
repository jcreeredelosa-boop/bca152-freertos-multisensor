# BCA152 FreeRTOS Multisensor — ESP32 Room Monitor

## Project Overview
Simulated ESP32 room-monitoring system built with ESP-IDF + FreeRTOS
on PlatformIO, simulated in Wokwi.

## Features
- DHT22 temperature & humidity
- LDR ambient light (0–100 %)
- PIR motion detection
- Rotary encoder user navigation
- SSD1306 OLED display (one measurement at a time)
- Temperature alarm via buzzer (<18 °C, >30 °C)
- ACTIVE / INACTIVE system state machine (15 s inactivity timeout)

## Learning Objectives
(see lab handout §2)

## System Architecture
(diagram)

## FreeRTOS Architecture
- SensorTask (prio 2, 2 s periodic via vTaskDelayUntil)
- DisplayTask (prio 1, owns OLED)
- InputTask (prio 3, encoder)
- MotionTask (prio 3, PIR)
- AlarmTask (prio 2, buzzer)
- StateTask (prio 2, ACTIVE/INACTIVE)

## IPC
- sensorQueue: SensorTask → DisplayTask
- alarmQueue : SensorTask → AlarmTask
- modeQueue  : InputTask  → DisplayTask
- serialMutex: protects Serial output
- systemEvents: EVENT_ACTIVE / EVENT_MOTION / EVENT_ALARM

## State Machine
ACTIVE --inactivity timeout--> INACTIVE
INACTIVE --motion detected--> ACTIVE

## Pin Configuration
| Signal | GPIO |
|---|---|
| DHT22 data | 4 |
| LDR (ADC1_CH6) | 34 |
| PIR OUT | 27 |
| Encoder A / B / SW | 32 / 33 / 25 |
| OLED SDA / SCL | 21 / 22 |
| Buzzer | 26 |

## Repository Structure
(see tree)

## Getting Started / Building / Running
```bash
pio run
pio run -t upload   # not needed for Wokwi
# In VS Code: Ctrl+Shift+P → "Wokwi: Start Simulator"