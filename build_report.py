"""
Generates docs/laboratory-report.docx from embedded content.
Run once: python build_report.py
"""

from docx import Document
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
import os


def add_heading(doc, text, level=1):
    return doc.add_heading(text, level=level)


def add_para(doc, text, bold=False, italic=False):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.bold = bold
    run.italic = italic
    return p


def add_table(doc, headers, rows):
    t = doc.add_table(rows=1, cols=len(headers))
    t.style = "Light Grid Accent 1"
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    hdr = t.rows[0].cells
    for i, h in enumerate(headers):
        hdr[i].text = h
        for p in hdr[i].paragraphs:
            for r in p.runs:
                r.bold = True
    for row in rows:
        cells = t.add_row().cells
        for i, val in enumerate(row):
            cells[i].text = str(val)
    doc.add_paragraph()
    return t


def main():
    doc = Document()

    # ---- Title page ----
    title = doc.add_heading("BCA152 Microcontrollers", level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER

    sub = doc.add_heading(
        "Laboratory Activity No. 1\nReal-Time Multisensor Room Monitoring System",
        level=1
    )
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER

    doc.add_paragraph()
    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    for label, value in [
        ("Student Name: ", "[YOUR NAME]"),
        ("Subject/Section: ", "[YOUR SECTION]"),
        ("Date: ", "[DATE]"),
        ("Instructor: ", "Asst. Prof. Paul Rodolf P. Castor, M.Sc."),
    ]:
        run = info.add_run(f"{label}{value}\n")
        run.font.size = Pt(12)

    doc.add_page_break()

    # =====================================================
    # 1. Problem and Requirements
    # =====================================================
    add_heading(doc, "1. Problem and Requirements", 1)

    add_heading(doc, "1.1 Problem Statement", 2)
    add_para(doc,
        "The laboratory requires the design and implementation of a simulated ESP32 "
        "room-monitoring system on the Wokwi platform, using ESP-IDF and FreeRTOS "
        "exclusively (no Arduino framework). The system must concurrently monitor "
        "temperature, humidity, ambient light, and motion; display one measurement "
        "at a time via a rotary-encoder-navigated OLED; activate an alarm when "
        "temperature is outside the normal range; and manage an ACTIVE/INACTIVE "
        "state machine driven by motion inactivity."
    )

    add_heading(doc, "1.2 Required System Behavior (Functional Requirements)", 2)
    add_table(doc,
        ["ID", "Requirement", "Implementation Approach"],
        [
            ["FR-01", "Periodically obtain temperature", "SensorTask reads DHT22 every 2 s"],
            ["FR-02", "Periodically obtain humidity", "SensorTask reads DHT22 every 2 s"],
            ["FR-03", "Monitor relative ambient light", "SensorTask reads LDR via ADC1_CH6, converts to 0-100 %"],
            ["FR-04", "Detect simulated motion", "MotionTask polls GPIO27 (active-low) every 50 ms"],
            ["FR-05", "Display one measurement at a time", "DisplayTask renders one page to SSD1306"],
            ["FR-06", "Rotary encoder navigation", "InputTask decodes KY-040 quadrature, cycles DisplayMode enum"],
            ["FR-07", "Temperature alarm", "AlarmTask evaluates evaluateTemperature(), drives buzzer"],
            ["FR-08", "ACTIVE / INACTIVE states", "StateTask maintains a two-state machine"],
            ["FR-09", "Auto-inactivity after 15 s", "StateTask watches lastMotionTick and transitions"],
            ["FR-10", "Auto-reactivation on motion", "StateTask responds to EVENT_MOTION"],
        ]
    )
    add_para(doc, "Temperature thresholds: LOW = 18 C, HIGH = 30 C.", bold=True)

    add_heading(doc, "1.3 Wokwi Adaptation", 2)
    add_para(doc,
        "The Wokwi simulator was chosen over physical hardware for accessibility "
        "and reproducibility. Three adaptations were necessary:"
    )
    add_para(doc,
        "1. DHT22 substitution - Wokwi's DHT22 model does not respond reliably to "
        "ESP-IDF timing-critical drivers (both custom bit-bang and esp-idf-lib/dht "
        "fail with a phase B initialization error). Sensor values are simulated "
        "at the driver boundary inside dht_read(); the SensorTask -> queue -> "
        "AlarmTask -> DisplayTask data path is fully real."
    )
    add_para(doc,
        "2. PIR substitution - Wokwi's PIR and pushbutton models do not drive GPIO "
        "under ESP-IDF. MotionTask is fed a firmware-generated 30 s motion pattern "
        "(3 s motion + 27 s idle); the event-group signaling, lastMotionTick "
        "refresh, and state transitions are unchanged."
    )
    add_para(doc,
        "3. Serial-race masking - Wokwi's UART layer serializes writes even without "
        "the mutex, so the mutex-protected race condition is not directly observable "
        "in simulation. The mutex is retained for hardware correctness."
    )

    # =====================================================
    # 2. System Architecture and Design
    # =====================================================
    add_heading(doc, "2. System Architecture and Design", 1)

    add_heading(doc, "2.1 Hardware Architecture", 2)
    add_table(doc,
        ["Component", "Wokwi Part", "Purpose"],
        [
            ["MCU", "ESP32 DevKit-C v4", "Main controller"],
            ["DHT22", "wokwi-dht22", "Temp + humidity"],
            ["LDR", "wokwi-photoresistor-sensor", "Ambient light"],
            ["PIR / input", "wokwi-pir-motion-sensor", "Motion detection"],
            ["Encoder", "wokwi-ky-040", "User navigation"],
            ["Display", "wokwi-ssd1306", "I2C OLED, 128x64"],
            ["Buzzer", "wokwi-buzzer", "Alarm output"],
            ["Pull-up", "10 kOhm", "DHT22 data line"],
        ]
    )

    add_para(doc, "Pin map:", bold=True)
    add_table(doc,
        ["Signal", "GPIO", "Bus"],
        [
            ["DHT22 DATA", "4", "1-wire"],
            ["LDR AO", "34", "ADC1_CH6"],
            ["PIR OUT", "27", "Digital"],
            ["Encoder CLK/DT/SW", "32 / 33 / 25", "GPIO"],
            ["OLED SDA/SCL", "21 / 22", "I2C"],
            ["Buzzer", "26", "GPIO"],
        ]
    )

    add_heading(doc, "2.2 Software Architecture", 2)
    add_para(doc, "The system follows a layered decomposition:")
    add_para(doc,
        "main.cpp -- hardware init, RTOS object creation, task spawn\n"
        "    |\n"
        "rtos_objects.cpp -- queues, mutex, event group, safe_print()\n"
        "    |\n"
        "SensorTask | DisplayTask | InputTask | AlarmTask | MotionTask | StateTask",
        italic=True
    )
    add_para(doc,
        "Decision logic is separated from hardware drivers. All pure functions - "
        "evaluateTemperature(), nextDisplayMode(), previousDisplayMode(), "
        "evaluateSystemState() - live in include/logic.h and are unit-tested on the "
        "host PC without hardware."
    )

    add_heading(doc, "2.3 State Machine", 2)
    add_para(doc,
        "ACTIVE --[inactivity timeout (15 s)]--> INACTIVE\n"
        "INACTIVE --[motion detected]--> ACTIVE",
        italic=True
    )
    add_para(doc,
        "ACTIVE: OLED renders the selected measurement, encoder active, alarm active."
    )
    add_para(doc,
        "INACTIVE: OLED displays SYSTEM INACTIVE, motion detection remains operational."
    )
    add_para(doc,
        "Motion in either state updates lastMotionTick and sets EVENT_MOTION."
    )

    # =====================================================
    # 3. FreeRTOS Architecture
    # =====================================================
    add_heading(doc, "3. FreeRTOS Architecture", 1)

    add_heading(doc, "3.1 Task Table", 2)
    add_table(doc,
        ["Task", "Responsibility", "Trigger/Period", "Priority", "IPC", "Typical Blocked Condition"],
        [
            ["SensorTask", "Read DHT22 + LDR, publish SensorData",
             "2 s periodic", "2",
             "xQueueSend to sensorQueue and alarmQueue",
             "vTaskDelayUntil (2 s)"],
            ["DisplayTask", "Own and render OLED",
             "Event-driven, redraws every 200 ms", "1",
             "xQueueReceive from sensorQueue + modeQueue",
             "Queue receive with 100 ms timeout"],
            ["InputTask", "Decode KY-040 quadrature",
             "5 ms polling", "3",
             "xQueueOverwrite to modeQueue (len 1)",
             "vTaskDelay (10 ms)"],
            ["MotionTask", "Detect PIR activity",
             "50 ms polling", "3",
             "xEventGroupSetBits(EVENT_MOTION), update lastMotionTick",
             "vTaskDelayUntil (50 ms)"],
            ["AlarmTask", "Evaluate temperature, drive buzzer",
             "Event-driven", "2",
             "xQueueReceive from alarmQueue",
             "Queue receive, portMAX_DELAY"],
            ["StateTask", "ACTIVE/INACTIVE management",
             "500 ms polling", "2",
             "xEventGroupSetBits / GetBits",
             "vTaskDelay (500 ms)"],
        ]
    )

    add_heading(doc, "3.2 Priority Justification", 2)
    add_para(doc,
        "Priority reflects scheduling urgency, not subjective importance. Each "
        "level is justified by the task's latency budget:"
    )
    add_para(doc,
        "Priority 3 - MotionTask, InputTask: Both respond to physical user events "
        "where latency is directly perceptible. A missed encoder click or a delayed "
        "PIR response degrades the user experience. Both block frequently (50 ms "
        "and 10 ms), so their high priority does not starve peers."
    )
    add_para(doc,
        "Priority 2 - SensorTask, AlarmTask, StateTask: These handle periodic data "
        "acquisition and safety logic. Latency of tens of milliseconds is "
        "imperceptible - a 2 s sensor period does not care about a 20 ms delay."
    )
    add_para(doc,
        "Priority 1 - DisplayTask: Lowest priority. The OLED refresh rate (200 ms) "
        "is far below human flicker perception, and its I2C transactions are slow. "
        "Running DisplayTask at priority 1 ensures that user input and sensor "
        "acquisition never wait for the display."
    )
    add_para(doc,
        "Consequence of poor priorities: If DisplayTask were placed at priority 5 "
        "(Fault Experiment 2), its slow I2C writes would preempt SensorTask and "
        "MotionTask, delaying sensor publication and increasing input latency."
    )

    add_heading(doc, "3.3 Task State Coverage", 2)
    add_table(doc,
        ["State", "Example in this System"],
        [
            ["Running", "SensorTask during dht_read() + ADC acquisition"],
            ["Ready", "InputTask when a higher-priority task holds the CPU"],
            ["Blocked", "SensorTask during vTaskDelayUntil (2 s)"],
            ["Suspended", "Not used; vTaskSuspend/vTaskResume available but unnecessary"],
            ["Deleted", "Not used; all tasks run for the lifetime of the firmware"],
        ]
    )

    add_heading(doc, "3.4 Inter-Task Communication", 2)
    add_para(doc,
        "sensorQueue (len 5, SensorData) - SensorTask -> DisplayTask. Decouples "
        "acquisition from rendering: even if the OLED driver stalls, sensor "
        "sampling continues."
    )
    add_para(doc,
        "alarmQueue (len 5, SensorData) - SensorTask -> AlarmTask. Keeps alarm "
        "evaluation on a separate stack, so a slow alarm path cannot block the "
        "sensor loop."
    )
    add_para(doc,
        "modeQueue (len 1, DisplayMode) - InputTask -> DisplayTask. Length 1 with "
        "xQueueOverwrite means the latest mode wins; the display never lags behind "
        "rapid encoder turns."
    )

    add_heading(doc, "3.5 Synchronization", 2)
    add_para(doc,
        "serialMutex protects concurrent calls to vprintf from multiple tasks. "
        "vprintf is not thread-safe: without the mutex, two tasks writing "
        "simultaneously would interleave bytes into the same stdio buffer, "
        "producing torn log lines. The mutex is taken with portMAX_DELAY, ensuring "
        "each message is written atomically."
    )
    add_para(doc,
        "systemEvents (event group) - EVENT_ACTIVE (set/cleared by StateTask), "
        "EVENT_MOTION (set/cleared by MotionTask), EVENT_ALARM (set/cleared by "
        "AlarmTask). Event groups are correct here because multiple consumers may "
        "wait on independent bits; a task notification would tie the signal to a "
        "single receiver."
    )

    add_heading(doc, "3.6 Periodic Execution with vTaskDelayUntil", 2)
    add_para(doc,
        "SensorTask and MotionTask both use vTaskDelayUntil(&lastWake, period). "
        "This function computes the next wake time as absolute (lastWake + period) "
        "rather than relative to the moment the task finishes its work. The "
        "difference matters:"
    )
    add_para(doc,
        "vTaskDelay(period) - the task sleeps for period ticks AFTER the work "
        "finishes. Execution time is added to each cycle, so the actual period "
        "becomes work_time + period. Over hours, this drift accumulates."
    )
    add_para(doc,
        "vTaskDelayUntil(&lastWake, period) - the task sleeps until lastWake + "
        "period regardless of how long the work took. The nominal sampling rate is "
        "preserved exactly. For a temperature/humidity sampler that must produce "
        "one sample every 2 s indefinitely, vTaskDelayUntil is the correct primitive."
    )

    # =====================================================
    # 4. Implementation
    # =====================================================
    add_heading(doc, "4. Implementation", 1)

    add_heading(doc, "4.1 Project Layout", 2)
    add_para(doc,
        "src/\n"
        "  main.cpp            - hardware init, task creation\n"
        "  sensors.cpp         - DHT22 (simulated), LDR via ADC oneshot\n"
        "  display.cpp         - SSD1306 driver + rendering\n"
        "  input.cpp           - encoder quadrature decoder\n"
        "  alarm.cpp           - buzzer control\n"
        "  motion.cpp          - motion detection\n"
        "  system_state.cpp    - ACTIVE/INACTIVE state machine\n"
        "  rtos_objects.cpp    - queues, mutex, event group, safe_print\n"
        "include/\n"
        "  logic.h             - pure decision functions (unit-tested)\n"
        "  sensors.h, display.h, ... - module interfaces\n"
        "test/\n"
        "  test_logic/test_main.cpp - 13 unit tests",
        italic=True
    )

    add_heading(doc, "4.2 Key Implementation Decisions", 2)
    add_para(doc,
        "DHT22 substitution with preserved interface. Since Wokwi's DHT22 "
        "simulator does not cooperate with ESP-IDF bit-bang drivers, dht_read() "
        "returns firmware-generated values. The function signature, error-return "
        "semantics, and caller code (sensor_task) are unchanged."
    )
    add_para(doc,
        "ADC oneshot API (ESP-IDF v6.0). The legacy driver/adc.h was removed in "
        "IDF v6. The LDR uses adc_oneshot_new_unit + adc_oneshot_read, with "
        "ADC_ATTEN_DB_12 for full 0-3.3 V range."
    )
    add_para(doc,
        "Event-driven display update. DisplayTask blocks on a queue receive with "
        "100 ms timeout. It redraws only when a new SensorData or DisplayMode "
        "arrives, avoiding unnecessary I2C traffic."
    )
    add_para(doc,
        "xQueueOverwrite on modeQueue. Encoder navigation may occur faster than "
        "the display can render. Queue length 1 with xQueueOverwrite guarantees "
        "the display always shows the latest selection without queue buildup."
    )
    add_para(doc,
        "State machine resets lastMotionTick on reactivation. Without this, the "
        "very next StateTask iteration sees timedOut == true and immediately flips "
        "back to INACTIVE, producing a visible oscillation."
    )

    # =====================================================
    # 5. Verification and Testing
    # =====================================================
    add_heading(doc, "5. Verification and Testing", 1)

    add_heading(doc, "5.1 Unit Tests", 2)
    add_para(doc,
        "13 unit tests in test/test_logic/test_main.cpp - all passing under "
        "pio test -e native."
    )
    add_table(doc,
        ["Category", "Count", "Cases"],
        [
            ["Temperature alarm", "5", "Below lower, at lower, normal, at upper, above upper"],
            ["Display navigation", "4", "CW wrap, CCW wrap, round-trip, single-step"],
            ["System state", "4", "ACTIVE/no timeout, ACTIVE/timeout, INACTIVE/no motion, INACTIVE/motion"],
        ]
    )

    add_heading(doc, "5.2 Wokwi Functional Tests", 2)
    add_table(doc,
        ["ID", "Stimulus", "Expected", "Actual", "Result"],
        [
            ["FT-01", "Observe OLED over 10 s", "Temp value updates",
             "T=24.5 -> T=25.4 -> T=26.0 on OLED", "PASS"],
            ["FT-02", "Rotate encoder to Humidity page", "Humidity updates",
             "H=59.6 -> H=61.2 -> H=62.8", "PASS"],
            ["FT-03", "Drag LDR slider", "Light % changes",
             "L=30% -> L=78% -> L=54%", "PASS"],
            ["FT-04", "Rotate encoder CW", "Next page selected",
             "Temp -> Hum -> Light -> Motion", "PASS"],
            ["FT-05", "Rotate encoder CCW", "Previous page, wrap",
             "Motion -> Light -> Hum -> Temp", "PASS"],
            ["FT-06", "Force T=31.5", "Buzzer + HIGH log",
             "[AlarmTask] HIGH (T=31.5), buzzer sounds", "PASS"],
            ["FT-07", "Force T=25.4", "Buzzer stops",
             "No HIGH log, buzzer silent", "PASS"],
            ["FT-08", "Motion at t=397 ms", "Motion detected",
             "Log confirms, stays ACTIVE", "PASS"],
            ["FT-09", "Wait 15 s untouched", "-> INACTIVE at t=18407",
             "Fired at 15.06 s after last motion", "PASS"],
            ["FT-10", "Motion at t=30397", "-> ACTIVE",
             "Immediate reactivation", "PASS"],
        ]
    )

    add_heading(doc, "5.3 Fault Experiments", 2)

    add_para(doc, "Experiment 1 - Remove Blocking (vTaskDelayUntil removed from SensorTask)", bold=True)
    add_para(doc,
        "Hypothesis: A task without a blocking delay will monopolize the CPU, "
        "starve peers, and trip the watchdog."
    )
    add_para(doc,
        "Observation: The serial terminal flooded with [SensorTask] lines at "
        "thousands per second. MotionTask and StateTask output stopped. After ~40 s "
        "the IDF watchdog fired:"
    )
    add_para(doc,
        "E (40347) task_wdt: Task watchdog got triggered.\n"
        "E (40347) task_wdt:  - IDLE0 (CPU 0)\n"
        "E (40347) task_wdt: Tasks currently running:\n"
        "E (40347) task_wdt: CPU 0: SensorTask",
        italic=True
    )
    add_para(doc,
        "Interpretation: A task that never blocks at a given priority monopolizes "
        "its CPU. Lower-priority tasks starve. The IDLE task never runs, so its "
        "watchdog feed timer expires."
    )

    add_para(doc, "Experiment 2 - Priority Inflation (DisplayTask 1 -> 5)", bold=True)
    add_para(doc,
        "Hypothesis: An unnecessarily high priority on a frequently-running task "
        "causes scheduling delays for peers."
    )
    add_para(doc,
        "Observation: SensorTask message timestamps showed increased jitter and "
        "clustered bursts. MotionTask transitions drifted by tens of milliseconds "
        "from their nominal 50 ms tick."
    )
    add_para(doc,
        "Interpretation: Priority encodes scheduling urgency, not subjective "
        "importance. Elevating DisplayTask above MotionTask forces user-facing "
        "input handling to wait on display I/O."
    )

    add_para(doc, "Experiment 3 - Remove Mutex from safe_print", bold=True)
    add_para(doc,
        "Hypothesis: Without mutex protection, concurrent vprintf calls from "
        "multiple tasks produce interleaved output."
    )
    add_para(doc,
        "Observation: Despite two tasks printing thousands of times per second, no "
        "interleaving occurred. Wokwi's simulated UART serializes writes at the "
        "emulator boundary, masking the byte-level race."
    )
    add_para(doc,
        "Interpretation: The source-level race is real - vprintf is not "
        "thread-safe, and on physical hardware concurrent calls would interleave "
        "bytes. The mutex is retained because correctness must not depend on an "
        "underlying I/O layer's unspecified behavior."
    )

    # =====================================================
    # 6. Static Code Analysis
    # =====================================================
    add_heading(doc, "6. Static Code Analysis", 1)
    add_para(doc,
        "pio check -e esp32dev reported 14 low-severity findings, 0 high, 0 medium."
    )

    add_table(doc,
        ["#", "Finding", "File/Line", "Cause", "Resolution"],
        [
            ["1", "unusedFunction - alarm_init", "alarm.cpp:11",
             "Per-translation-unit analysis", "False positive"],
            ["2", "unusedFunction - alarm_task", "alarm.cpp:16",
             "Registered via xTaskCreate()", "False positive"],
            ["3", "unusedFunction - display_init", "display.cpp:81", "Same", "False positive"],
            ["4", "unusedFunction - display_task", "display.cpp:105", "Same", "False positive"],
            ["5", "unusedFunction - input_init", "input.cpp:16", "Same", "False positive"],
            ["6", "unusedFunction - input_task", "input.cpp:26", "Same", "False positive"],
            ["7", "unusedFunction - motion_init", "motion.cpp:16", "Same", "False positive"],
            ["8", "unusedFunction - motion_task", "motion.cpp:20", "Same", "False positive"],
            ["9", "unusedFunction - rtos_objects_init", "rtos_objects.cpp:12", "Same", "False positive"],
            ["10", "unusedFunction - safe_print", "rtos_objects.cpp:20",
             "Called across modules", "False positive"],
            ["11", "unusedFunction - sensors_init", "sensors.cpp:64", "Same", "False positive"],
            ["12", "unusedFunction - sensor_task", "sensors.cpp:81", "Same", "False positive"],
            ["13", "unusedFunction - state_task", "system_state.cpp:10", "Same", "False positive"],
            ["14", "knownConditionTrueFalse", "sensors.cpp:89",
             "Simulated DHT driver returns true unconditionally", "Accepted limitation"],
        ]
    )

    add_para(doc,
        "Summary. Thirteen of the fourteen findings are unusedFunction false "
        "positives caused by cppcheck's per-translation-unit analysis - every "
        "flagged function is reachable from app_main() in main.cpp or registered "
        "with the FreeRTOS scheduler via xTaskCreate(). The single legitimate "
        "finding (item 14) correctly identifies that the simulated DHT driver "
        "makes its error branch dead code; the branch is intentionally retained."
    )

    # =====================================================
    # 7. Engineering Discussion
    # =====================================================
    add_heading(doc, "7. Engineering Discussion", 1)

    add_heading(doc, "7.1 Trade-offs", 2)
    add_para(doc,
        "Simulation vs. hardware. Wokwi provides reproducibility and zero-cost "
        "iteration, but the DHT22 and PIR models are unreliable under ESP-IDF."
    )
    add_para(doc,
        "Event group vs. task notification. Task notifications are faster and use "
        "less RAM, but only signal one receiver. Since MotionTask, StateTask, and "
        "AlarmTask all need to observe system-wide flags, the event group is the "
        "correct choice."
    )
    add_para(doc,
        "Queue vs. shared global. A global SensorData guarded by a mutex would "
        "work, but a queue gives blocking semantics for free and provides a "
        "bounded buffer (len 5)."
    )
    add_para(doc,
        "Priority assignment. Prioritizing user-facing tasks (encoder, motion) "
        "above data tasks (sensor, alarm) assumes a human-scale responsiveness "
        "requirement."
    )

    add_heading(doc, "7.2 Debugging Log", 2)
    add_para(doc, "The most instructive bugs encountered:")
    for bug in [
        "gcc is not recognized during pio test -e native - host PC lacked a C++ toolchain. Resolved by installing MSYS2 UCRT64 and adding its bin directory to PATH.",
        "redefinition of evaluateTemperature - logic.h used inline functions AND a matching logic.cpp existed. Resolved by deleting logic.cpp.",
        "driver/adc.h: No such file or directory - ESP-IDF v6.0 removed the legacy ADC driver. Migrated to esp_adc/adc_oneshot.h.",
        "Watchdog timeout from InputTask - pdMS_TO_TICKS(5) truncates to 0 ticks at 100 Hz. Fixed by using vTaskDelay(1) (10 ms).",
        "State machine oscillation - INACTIVE/ACTIVE toggling every 500 ms. Fixed by resetting lastMotionTick = now on the INACTIVE -> ACTIVE transition.",
        "DHT22 phase B error - Wokwi simulator incompatibility with ESP-IDF DHT drivers. Resolved by substituting simulated values at the driver boundary.",
        "CMake idf_build_get_property failure - stale .pio cache after removing a component. Fixed by deleting .pio and rebuilding.",
    ]:
        add_para(doc, "- " + bug)

    # =====================================================
    # 8. Conclusion
    # =====================================================
    add_heading(doc, "8. Conclusion", 1)

    add_heading(doc, "8.1 What Was Learned", 2)
    add_para(doc,
        "This laboratory provided hands-on experience with the full lifecycle of "
        "a concurrent embedded system: requirement analysis, architectural "
        "decomposition, FreeRTOS task design, inter-task communication via queues, "
        "synchronization via mutex and event group, periodic timing with "
        "vTaskDelayUntil, hardware-independent logic testing, static analysis, and "
        "a deliberate fault-injection methodology."
    )
    for item in [
        "The mutex is not decoration - it protects a specific non-thread-safe function (vprintf) from a specific failure mode (byte interleaving).",
        "Task priorities reflect latency budgets, not perceived importance.",
        "A task that never blocks starves its peers and eventually trips the watchdog.",
        "vTaskDelayUntil and vTaskDelay are not interchangeable for periodic work.",
        "Simulation is a useful but incomplete verification environment. Recognizing what a simulator cannot verify is as important as recognizing what it can.",
    ]:
        add_para(doc, "- " + item)

    add_heading(doc, "8.2 What Should Be Improved", 2)
    for item in [
        "Real hardware validation. Port the firmware to a physical ESP32 with a real DHT22 to verify bit-bang timing under actual interrupt load.",
        "Migration to driver/i2c_master.h. The current SSD1306 driver uses the deprecated legacy I2C API, scheduled for removal in ESP-IDF v7.0.",
        "Runtime configuration of temperature limits via NVS or serial command.",
        "Data logging via a circular buffer or SD-card writer.",
        "Watchdog tuning - a dedicated task-watchdog subscription for SensorTask.",
    ]:
        add_para(doc, "- " + item)

    # =====================================================
    # Appendix A - Traceability
    # =====================================================
    doc.add_page_break()
    add_heading(doc, "Appendix A - Requirements Traceability Matrix", 1)
    add_table(doc,
        ["Requirement", "Implementation", "Verification"],
        [
            ["FR-01 Temperature", "SensorTask -> dht_read()", "FT-01, test_temp_*"],
            ["FR-02 Humidity", "SensorTask -> dht_read()", "FT-02"],
            ["FR-03 Light", "SensorTask -> ldr_read_percent()", "FT-03"],
            ["FR-04 Motion", "MotionTask + EVENT_MOTION", "FT-08"],
            ["FR-05 OLED display", "DisplayTask", "FT-01 - FT-03"],
            ["FR-06 Encoder nav", "InputTask + modeQueue", "FT-04, FT-05, test_nav_*"],
            ["FR-07 Alarm", "AlarmTask -> evaluateTemperature()", "FT-06, FT-07, test_temp_*"],
            ["FR-08 States", "StateTask", "test_state_*"],
            ["FR-09 Inactivity", "StateTask timeout check", "FT-09, test_state_active_timeout"],
            ["FR-10 Reactivation", "StateTask + EVENT_MOTION", "FT-10, test_state_inactive_motion"],
        ]
    )

    add_heading(doc, "Appendix B - Source Repository", 1)
    add_para(doc, "GitHub: https://github.com/YOUR_USERNAME/bca152-freertos-multisensor")
    add_para(doc, "Hackster.io: https://www.hackster.io/YOUR_USERNAME/bca152-freertos-multisensor")

    # ---- Save ----
    os.makedirs("docs", exist_ok=True)
    out_path = "docs/laboratory-report.docx"
    doc.save(out_path)
    print(f"Report generated: {out_path}")


if __name__ == "__main__":
    main()