# Assignment 1: Priority Inversion

**Course:** Real Time Systems (4\_EFS8050), Lecture 11
**Platform:** ESP32-S3 DevKitC-1 in Wokwi (course demo: https://wokwi.com/projects/480870073351352321), Serial Monitor 115200 baud
**Files:** `Demo3\_Priority\_Inversion/Demo3\_Priority\_Inversion.ino` (modified sketch), this README

|Task|Priority|Behaviour|
|-|-|-|
|Task\_L|1 (low)|Takes the lock, then three 100 ms work sections|
|Task\_H|3 (high)|Requests the same lock at about 100 ms|
|Task\_M|2 (medium)|Starts at about 150 ms, 1000 ms of work, no lock|

\---

## Part A: Predictions

**Binary semaphore: what happens after Task\_H requests the lock?**
At 100 ms Task\_L holds the semaphore and has done about 100 ms of its 300 ms of work. Task\_H blocks. Task\_L continues, but at 150 ms Task\_M wakes up. Task\_M (priority 2) is higher than Task\_L (priority 1), so it preempts Task\_L and runs its full 1000 ms. Task\_L cannot finish and cannot give the semaphore back, so Task\_H keeps waiting for unrelated medium-priority work. This is unbounded priority inversion.

Timeline (binary semaphore, approximate):

```
time (ms)  0        100  150                          1150      1300  1350
Task\_H     .........|req BLOCKED......................................|run|
Task\_M     ..............|========= 1000 ms work =========|...............
Task\_L     |== work ==|==|....... preempted by M ........|== work ==|give
                                         -> H waits about 1200 ms
```

**Mutex with priority inheritance: what changes?**
When Task\_H blocks on the mutex held by Task\_L, FreeRTOS temporarily raises Task\_L to priority 3. At 150 ms Task\_M wakes, but priority 2 is lower than Task\_L's inherited 3, so Task\_M cannot preempt it. Task\_L finishes its remaining work (about 200 ms), gives the mutex and drops back to priority 1. Task\_H takes the mutex right away, and Task\_M runs after Task\_H.

Timeline (mutex, approximate):

```
time (ms)  0        100                 300 350
Task\_H     .........|req BLOCKED........|run|
Task\_L     |== p1 ==|=== p3 (inherited)=|give
Task\_M     ..............(ready, waits).....|===== 1000 ms work =====|
                                -> H waits about 200 ms
```

**Which task inherits a higher priority, and which priority?**
Task\_L inherits priority 3 from Task\_H while Task\_H is waiting for the mutex.

\---

## Part B: Measurements

### Step 1: Binary semaphore (USE\_MUTEX 0, TASK\_M\_WORK\_MS 1000)

```


=== Demo 3: Priority Inversion | USE\_MUTEX 0 (binary semaphore, NO inheritance) ===

\[    0 ms] L: taking lock

\[  101 ms] H: needs the lock

\[  101 ms] L: critical section 1/3, my priority = 1

\[  151 ms] M: starts 1000 ms of work (does not use the lock)

\[ 1151 ms] M: finished

\[ 1201 ms] L: critical section 2/3, my priority = 1

\[ 1301 ms] L: critical section 3/3, my priority = 1

\[ 1301 ms] H: got the lock after waiting 1200 ms

\[ 1351 ms] H: done


```

### Step 2: Mutex (USE\_MUTEX 1, TASK\_M\_WORK\_MS 1000)

```


=== Demo 3: Priority Inversion | USE\_MUTEX 1 (mutex, priority inheritance) ===

\[    0 ms] L: taking lock

\[  101 ms] H: needs the lock

\[  101 ms] L: critical section 1/3, my priority = 3

\[  201 ms] L: critical section 2/3, my priority = 3

\[  301 ms] L: critical section 3/3, my priority = 3

\[  301 ms] H: got the lock after waiting 200 ms

\[  351 ms] H: done

\[  351 ms] M: starts 1000 ms of work (does not use the lock)

\[ 1351 ms] M: finished

```

### Comparison

|Observation|Binary semaphore (USE\_MUTEX 0)|Mutex (USE\_MUTEX 1)|
|-|-|-|
|Task\_H waiting time|1200 ms|200 ms|
|Task\_L printed priority|1|3 (while H waits)|
|Completion order|M, L, H|L, H, M|
|Does Task\_M delay Task\_L?|Yes|No|
|Does priority inheritance occur?|No|Yes|

**Compared with my predictions:** Both results matched my predictions exactly: about 1200 ms of waiting with the binary semaphore and about 200 ms with the mutex. With the mutex, Task\_L's printed priority changed from 1 to 3, which proves that it inherited Task\_H's priority.

### Step 3: Task\_M workload 1000 → 2000 ms

**Prediction (before running):** With the binary semaphore, Task\_H's wait grows by about the extra 1000 ms (to roughly 2200 ms), because Task\_H waits for all of Task\_M's work. With the mutex, the wait stays about 200 ms, because Task\_M cannot preempt Task\_L while Task\_L has inherited priority 3.

USE\_MUTEX 0, TASK\_M\_WORK\_MS 2000:

```
=== Demo 3: Priority Inversion | USE\_MUTEX 0 (binary semaphore, NO inheritance) ===

\[    0 ms] L: taking lock

\[  101 ms] H: needs the lock

\[  101 ms] L: critical section 1/3, my priority = 1

\[  151 ms] M: starts 1000 ms of work (does not use the lock)

\[ 2151 ms] M: finished

\[ 2201 ms] L: critical section 2/3, my priority = 1

\[ 2301 ms] L: critical section 3/3, my priority = 1

\[ 2301 ms] H: got the lock after waiting 2200 ms

\[ 2351 ms] H: done
```

USE\_MUTEX 1, TASK\_M\_WORK\_MS 2000:

```
=== Demo 3: Priority Inversion | USE\_MUTEX 1 (mutex, priority inheritance) ===

\[    0 ms] L: taking lock

\[  101 ms] H: needs the lock

\[  101 ms] L: critical section 1/3, my priority = 3

\[  201 ms] L: critical section 2/3, my priority = 3

\[  301 ms] L: critical section 3/3, my priority = 3

\[  301 ms] H: got the lock after waiting 200 ms

\[  351 ms] H: done

\[  351 ms] M: starts 1000 ms of work (does not use the lock)

\[ 2351 ms] M: finished
```

||Binary semaphore|Mutex|
|-|-|-|
|H wait, M = 1000 ms|1200 ms |200 ms |
|H wait, M = 2000 ms|2200 ms |200 ms|

**Note: the "starts 1000 ms of work" text in the output is only the print label. The workload was changed to busyWorkMs(2000), which the M finish times (2151 ms and 2351 ms) confirm.**

**Why the two configurations react differently:** With a binary semaphore, Task\_H's waiting time depends on how long Task\_M runs, so it is unbounded: more medium-priority work means a longer wait. With a mutex, Task\_H only waits for the rest of Task\_L's critical section (bounded inversion), so Task\_M's workload does not matter.

\---

## Part C: Explanation

**Why does Task\_H wait longer with a binary semaphore?**
A binary semaphore has no owner, so FreeRTOS does not change any priority. Task\_L stays at priority 1. Task\_M has priority 2, so it can **preempt** Task\_L and run its full workload. Task\_L cannot release the lock until Task\_M finishes, so Task\_H, the highest **priority** task, ends up waiting for the medium task.

**How does Task\_L inherit Task\_H's priority with a mutex?**
A mutex has an owner. When Task\_H blocks on a mutex owned by Task\_L, FreeRTOS sees that a priority 3 task is waiting for a priority 1 owner. Task\_L then **inherits** priority 3 until it gives the mutex back. After that it returns to priority 1.

**Why can Task\_M no longer preempt Task\_L?**
Task\_M has **priority** 2. While Task\_L has **inherited** priority 3, Task\_L is the higher-priority task, so the scheduler will not let Task\_M **preempt** it. Task\_M stays Ready until Task\_L and Task\_H are finished.

**Main difference between a binary semaphore and a mutex here:**
A FreeRTOS mutex supports priority inheritance; a binary semaphore does not. A semaphore is for signalling events (for example from an ISR to a task). A mutex is for protecting a shared resource, so use a mutex whenever tasks of different priorities share a lock.

\---

## What I learned

* Priority inversion can make a high-priority task wait for unrelated medium-priority work. This is what reset the Mars Pathfinder.
* Changing one line (`xSemaphoreCreateBinary` → `xSemaphoreCreateMutex`) turns unbounded inversion into a short, bounded wait.
* Measure, don't assume: Task\_L's printed priority is direct proof that inheritance happened.

## Use of AI

I used an AI assistant (Claude) to help structure this README and draft the explanations and timelines. I ran all experiments myself in the Wokwi ESP32-S3 simulator, copied the real Serial Monitor output, filled in the tables with my own measurements, and checked that the explanations match my results.

