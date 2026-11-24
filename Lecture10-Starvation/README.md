# Assignment 2: Starvation

**Course:** Real Time Systems (4\_EFS8050), Lecture 10
**Platform:** ESP32-S3 DevKitC-1 in Wokwi (course demo: https://wokwi.com/projects/480848978582159361), Serial Monitor 115200 baud
**Files:** `Demo2\_Starvation/Demo2\_Starvation.ino` (modified sketch), this README

\---

## Part A: Understanding the problem

|Task|Priority|Behaviour|
|-|-|-|
|Task\_Control|3|Reads and processes a sensor continuously, no delay|
|Task\_Display|2|Updates every 200 ms, then `vTaskDelay()`|
|Task\_Log|1|Logs every second, then `vTaskDelay()`|

**1. Which tasks may experience starvation?**
Task\_Display and Task\_Log. FreeRTOS always runs the highest-priority task that is Ready. Task\_Control has the highest priority and never blocks, so it is always Ready and always chosen. Task\_Display and Task\_Log become Ready when their delays end, but they never get the CPU. They are not blocked. They are Ready but starved.

**2. Why is it dangerous for a high-priority task to run without blocking?**
On that core, every lower-priority task stops making progress. The display freezes, logs stop, and on ESP32 the idle task can also be starved, which can trigger the task watchdog. The system looks alive, but most of its functions have stopped.

**3. Two ways to give lower-priority tasks CPU time without changing priorities:**

* Make Task\_Control block regularly, for example with `vTaskDelay()` or `vTaskDelayUntil()` between sensor reads, so it runs at a fixed rate instead of constantly.
* Make Task\_Control event-driven: let it wait on a queue, semaphore or task notification (for example from a timer or sensor interrupt) instead of polling. It then only uses the CPU when there is new data.

\---

## Part B: ESP32 experiment

### Step 1: Observe starvation (STARVATION\_FIX 0)

Serial Monitor output (include both BUSY and SLEEP periods):

```
=== Demo 2: Starvation | STARVATION\_FIX 0 ===

t= 500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=1000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=1500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=2000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=2500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=3000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=3500 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=4000 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=4500 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=5000 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=5500 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=6000 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=6500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=7000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=7500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=8000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=8500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=9000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=9500 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=10000 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=10500 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

t=11000 ms | Task\_Hog: BUSY  | Task\_Low progress: +0

```

* **Task\_Low progress while Task\_Hog is BUSY:** \*\* +0 every 500 ms (expected about +0)
* **Task\_Low progress while Task\_Hog is SLEEP:** \*\* +500 every 500 ms (expected about +500)
* **What this shows:** While Task\_Hog (priority 2) busy-waits without ever blocking, Task\_Low (priority 1) is Ready but gets no CPU time, so its counter does not move. As soon as Task\_Hog sleeps, Task\_Low runs normally. This is starvation: the system is not frozen (Task\_Monitor, priority 3, still prints), but one task gets no service.

### Step 2: Enable the fix (STARVATION\_FIX 1, `vTaskDelay(1)`)

Serial Monitor output:

```


=== Demo 2: Starvation | STARVATION\_FIX 1 ===

t= 500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=1000 ms | Task\_Hog: BUSY  | Task\_Low progress: +45

t=1500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=2000 ms | Task\_Hog: BUSY  | Task\_Low progress: +45

t=2500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=3000 ms | Task\_Hog: SLEEP | Task\_Low progress: +54

t=3500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=4000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=4500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=5000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=5500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=6000 ms | Task\_Hog: BUSY  | Task\_Low progress: +490

t=6500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=7000 ms | Task\_Hog: BUSY  | Task\_Low progress: +45

t=7500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=8000 ms | Task\_Hog: BUSY  | Task\_Low progress: +45

t=8500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=9000 ms | Task\_Hog: SLEEP | Task\_Low progress: +54

t=9500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=10000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=10500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=11000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=11500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=12000 ms | Task\_Hog: BUSY  | Task\_Low progress: +490

t=12500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

t=13000 ms | Task\_Hog: BUSY  | Task\_Low progress: +45

t=13500 ms | Task\_Hog: BUSY  | Task\_Low progress: +46

```

**Comparison:** During BUSY, Task\_Low now makes progress of about +45 to +46 per 500 ms instead of +0. During SLEEP it is still about +500.

**Why Task\_Low can now run:** Every 10 ms Task\_Hog calls `vTaskDelay(1)` and moves to the Blocked state for one tick. While it is blocked, Task\_Low is the highest-priority Ready task on core 1, so the scheduler runs it. Task\_Hog still uses most of the CPU, but no longer all of it.

### Step 3: Change the delay to `vTaskDelay(10)`

**Prediction (written before running):** Task\_Hog will now block for 10 ticks after every 10 ms of work. On ESP32 one tick is normally 1 ms, so Task\_Hog will be blocked roughly half the time during BUSY. I expect Task\_Low's BUSY-period progress to increase a lot, to roughly half of the SLEEP value (about +250 per 500 ms).

Serial Monitor output:

```


=== Demo 2: Starvation | STARVATION\_FIX 1 ===

t= 500 ms | Task\_Hog: BUSY  | Task\_Low progress: +250

t=1000 ms | Task\_Hog: BUSY  | Task\_Low progress: +250

t=1500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=2000 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=2500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=3000 ms | Task\_Hog: SLEEP | Task\_Low progress: +259

t=3500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=4000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=4500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=5000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=5500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=6000 ms | Task\_Hog: BUSY  | Task\_Low progress: +490

t=6500 ms | Task\_Hog: BUSY  | Task\_Low progress: +250

t=7000 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=7500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=8000 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=8500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=9000 ms | Task\_Hog: SLEEP | Task\_Low progress: +259

t=9500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=10000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=10500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=11000 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=11500 ms | Task\_Hog: SLEEP | Task\_Low progress: +500

t=12000 ms | Task\_Hog: BUSY  | Task\_Low progress: +490

t=12500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=13000 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=13500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=14000 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

t=14500 ms | Task\_Hog: BUSY  | Task\_Low progress: +249

```

**Observation:**During BUSY, Task\_Low progress was +249 to +250 per 500 ms (compared with +45 to +46 with vTaskDelay(1) and +0 with no fix). During SLEEP it stayed at +500.

**Explanation:** The result matched my prediction. Task\_Hog now works for 10 ms and then stays Blocked for 10 ticks (10 ms on the ESP32, where one tick is 1 ms), so it uses the CPU only about half of the time during BUSY. Task\_Low gets the other half, which is why its progress is about half of the SLEEP value. The longer the high-priority task blocks, the more CPU time the lower-priority task receives.What I learned

* A Ready task is not guaranteed CPU time. With fixed-priority preemptive scheduling, a higher-priority task that never blocks can starve everything below it.
* Starvation is different from deadlock: the starved task is Ready, not Blocked, and the rest of the system keeps running.
* The fix is to make high-priority tasks block or wait for events. How long they block directly controls how much time the lower-priority tasks get.

## Use of AI

*I used an AI assistant (Claude) to help structure this README and check my explanations. I ran the experiments myself in Wokwi and copied the real output.*

