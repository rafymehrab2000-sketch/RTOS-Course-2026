# Assignment 1: Deadlock

**Course:** Real Time Systems (4\_EFS8050), Lecture 10
**Platform:** ESP32-S3 DevKitC-1 in Wokwi (course demo: https://wokwi.com/projects/480848656039044097), Serial Monitor 115200 baud
**Files:** `Demo1\_Deadlock/Demo1\_Deadlock.ino` (modified sketch), this README

\---

## Part A: Understanding the problem

|Task|Lock sequence|
|-|-|
|Task\_Sensor|i2cMutex → uartMutex|
|Task\_Logger|uartMutex → i2cMutex|
|Task\_Display|i2cMutex only|

**1. Which two tasks can cause a deadlock?**
Task\_Sensor and Task\_Logger. They take the same two mutexes in opposite order. Task\_Sensor can take `i2cMutex` while, at the same moment, Task\_Logger takes `uartMutex`. Then Task\_Sensor waits for `uartMutex`, which Task\_Logger holds, and Task\_Logger waits for `i2cMutex`, which Task\_Sensor holds. Each one waits for the other, so neither can ever continue. This is a circular wait.

**2. Can Task\_Display create a circular wait by itself?**
No. Task\_Display only takes one mutex and never holds one lock while waiting for a second one. It may have to wait for `i2cMutex` for a while, but it is not part of a cycle. When the owner releases the mutex, Task\_Display can continue.

**3. Which Coffman condition is removed by using the same lock order?**
Circular wait. If every task takes `i2cMutex` before `uartMutex`, no task can hold `uartMutex` while waiting for `i2cMutex`, so the cycle cannot form. The other three conditions (mutual exclusion, hold and wait, no preemption) are still true, but all four are needed for deadlock, so breaking one is enough.

\---

## Part B: ESP32 experiment

### Step 1: Observe the deadlock (DEMO\_MODE 0, original opposite lock order)

Serial Monitor output (about 8 lines):

```
== Demo 1: Deadlock | DEMO\_MODE 0 ===

\[Task\_1] taking A ...

\[Task\_1] has A, now needs B

\[Task\_2] taking B ...

\[Task\_2] has B, now needs A

\[HEARTBEAT] t=2 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=3 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=4 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=5 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=6 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=7 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=8 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=9 s | rounds: Task\_1=0 Task\_2=0

\[HEARTBEAT] t=10 s | rounds: Task\_1=0 Task\_2=0

```

**What happens to the worker tasks?**
Both tasks print that they hold their first mutex (Task\_1 has A, Task\_2 has B) and that they now need the other one. After that they print nothing more. Both are blocked forever in `xSemaphoreTake(..., portMAX\_DELAY)`, waiting for a mutex the other task holds.

**Which HEARTBEAT numbers stop changing?**
The round counters `Task\_1=` and `Task\_2=` stay at 0. Only the time `t=` keeps increasing, because `loop()` is a separate task that does not need the mutexes. So the board is not frozen: only the two worker tasks are deadlocked.

### Step 2: Fix the lock order (DEMO\_MODE 0, plan2 changed to A → B)

Change in `setup()`:

```cpp
// before: initPlan(plan2, "Task\_2", mutexB, "B", mutexA, "A");
initPlan(plan2, "Task\_2", mutexA, "A", mutexB, "B");
```

Serial Monitor output (about 8 lines):

```
=== Demo 1: Deadlock | DEMO\_MODE 0 ===

\[Task\_1] taking A ...

\[Task\_1] has A, now needs B

\[Task\_2] taking A ...

\[Task\_1] has BOTH locks -> work done (round 1)

\[Task\_2] has A, now needs B

\[Task\_1] taking A ...

\[Task\_2] has BOTH locks -> work done (round 1)

\[HEARTBEAT] t=2 s | rounds: Task\_1=4 Task\_2=3

\[HEARTBEAT] t=3 s | rounds: Task\_1=7 Task\_2=6

\[HEARTBEAT] t=4 s | rounds: Task\_1=10 Task\_2=10```

**Why can the tasks now continue?**
Both tasks now ask for mutex A first. Only one of them can get A. The other waits for A while holding nothing. The task with A then also gets B, does its work and releases both. Then the other task can run. Nobody holds one lock while waiting for a lock the other task holds, so there is no circular wait. Both round counters in the HEARTBEAT increase.

### Step 3: Timeout and backoff (DEMO\_MODE 1, original opposite lock order)

Serial Monitor output:

```
=== Demo 1: Deadlock | DEMO\_MODE 1 ===

\[Task\_1] taking A ...

\[Task\_1] has A, now needs B

\[Task\_2] taking B ...

\[Task\_2] has B, now needs A

\[Task\_1] TIMEOUT waiting for B -> release A, back off

\[Task\_2] has BOTH locks -> work done (round 1)

\[Task\_2] taking B ...

\[Task\_2] has B, now needs A

\[Task\_2] has BOTH locks -> work done (round 2)

\[HEARTBEAT] t=2 s | rounds: Task\_1=0 Task\_2=2

...

\[Task\_2] TIMEOUT waiting for A -> release B, back off

\[Task\_1] has BOTH locks -> work done (round 1)

\[HEARTBEAT] t=7 s | rounds: Task\_1=1 Task\_2=10

\[HEARTBEAT] t=12 s | rounds: Task\_1=10 Task\_2=11
```

**Explanation:**
When a task cannot get its second mutex within 500 ms, `xSemaphoreTake` returns `pdFALSE` and the task prints `TIMEOUT`. It must then release its first mutex, because keeping it would still block the other task and the deadlock would continue. Releasing it breaks the "hold and wait" situation. The random backoff (100–400 ms) makes the two tasks wait different amounts of time before retrying, so they do not grab their first locks at exactly the same moment again and again. Without it, the system could end up in livelock: the tasks keep timing out and retrying but never finish any work.

\---

## What I learned

* Deadlock needs all four Coffman conditions. The easiest one to break in practice is circular wait: always take mutexes in the same global order.
* A deadlock does not freeze the whole system. Other tasks (here the heartbeat) keep running, which can hide the problem. A progress counter is a simple way to detect it.
* Timeouts let a task detect the problem and recover, but only if it releases what it already holds. Random backoff prevents livelock.

## Use of AI

&#x20;*I used an AI assistant (Claude) to help structure this README and to explain the Coffman conditions. I ran all experiments myself in Wokwi, copied the real Serial Monitor output, and checked every explanation against my own results and the lecture notes.*

