# Laboratory Report Outline

Use this file only as a working outline. The final submission required by the laboratory is:

`docs/laboratory-report.pdf`

## 1. Problem and Requirements
Explain the room-monitoring problem, functional requirements FR-01 to FR-10, and how the physical components were adapted to Wokwi.

## 2. System Architecture and Design
Include:
- hardware architecture;
- software architecture;
- subsystem decomposition;
- ACTIVE/INACTIVE state machine;
- Wokwi circuit image;
- FreeRTOS architecture diagram.

## 3. FreeRTOS Architecture
Document each task's:
- responsibility;
- period/event;
- priority;
- IPC;
- blocked condition;
- expected task states.

Explain why `vTaskDelayUntil()` is used for periodic sensor sampling and how blocking affects task states.

## 4. Implementation
Discuss significant engineering decisions:
- custom DHT22 timing driver;
- ADC-based relative light measurement;
- single-owner OLED design;
- queues for latest sensor data;
- event-group signaling;
- serial mutex;
- alarm decision logic separated from buzzer hardware.

Do not narrate every source line.

## 5. Verification and Testing
Include:
- `pio test -e native` results;
- Wokwi functional verification FT-01 to FT-10;
- fault experiment results.

## 6. Static Code Analysis
Include actual `pio check` findings, severity, interpretation, and corrective action.

## 7. Engineering Discussion
Discuss:
- trade-offs;
- limitations;
- debugging problems;
- alternative approaches;
- changes made during development.

## 8. Conclusion
Summarize what was learned and what should be improved.

## Requirements Traceability

| Requirement | Implementation | Verification |
|---|---|---|
| FR-01 Temperature | SensorTask / DHT22 | FT-01 |
| FR-02 Humidity | SensorTask / DHT22 | FT-02 |
| FR-03 Ambient light | SensorTask / ADC1 | FT-03 |
| FR-04 Motion | MotionTask / PIR | FT-08–FT-10 |
| FR-05 OLED | DisplayTask | FT-01–FT-03 |
| FR-06 Encoder | InputTask | FT-04–FT-05 + unit tests |
| FR-07 Alarm | AlarmTask + evaluateTemperature() | FT-06–FT-07 + unit tests |
| FR-08 ACTIVE/INACTIVE | StateTask | FT-08–FT-10 + unit tests |
| FR-09 Inactivity | StateTask | FT-09 |
| FR-10 Reactivation | StateTask | FT-10 |

## Actual Verification Record

Do not fill `PASS` unless the observed behavior was actually recorded.

| Test ID | Input/Stimulus | Expected | Actual | Result |
|---|---|---|---|---|
| FT-01 | | | | |
| FT-02 | | | | |
| FT-03 | | | | |
| FT-04 | | | | |
| FT-05 | | | | |
| FT-06 | | | | |
| FT-07 | | | | |
| FT-08 | | | | |
| FT-09 | | | | |
| FT-10 | | | | |
