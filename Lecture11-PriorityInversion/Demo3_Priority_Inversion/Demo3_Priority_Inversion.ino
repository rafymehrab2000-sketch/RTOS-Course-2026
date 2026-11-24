/*
Real Time Systems (4_EFS8050) - Lecture 11
Demo 3 - Priority Inversion and Priority Inheritance
(MODIFIED for Assignment 1: Priority Inversion)

CHANGES MADE:
  Step 3: Task_M workload changed from busyWorkMs(1000) to busyWorkMs(2000)
          (set with TASK_M_WORK_MS below so both values are easy to test).

HOW TO RUN (Wokwi or ESP32-S3 DevKitC-1)
  Serial Monitor: 115200 baud.
  Step 1: USE_MUTEX 0, TASK_M_WORK_MS 1000
  Step 2: USE_MUTEX 1, TASK_M_WORK_MS 1000
  Step 3: TASK_M_WORK_MS 2000 with USE_MUTEX 0, then USE_MUTEX 1

SCENARIO (all tasks pinned to core 1, so priorities decide who runs)
  Task_L priority 1: takes the lock at t=0, works 300 ms inside it
  Task_H priority 3: wakes at t=100 ms and needs the same lock
  Task_M priority 2: wakes at t=150 ms, works TASK_M_WORK_MS, does NOT use the lock
*/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define USE_MUTEX 0          // 0 = binary semaphore (inversion), 1 = mutex (inheritance)
#define TASK_M_WORK_MS 2000  // original value: 1000

SemaphoreHandle_t lockHandle;
uint32_t t0;

// Burns CPU time in 10 ms slices, so time spent preempted is not counted as work.
void busyWorkMs(uint32_t ms) {
  for (uint32_t done = 0; done < ms; done += 10) {
    uint32_t s = millis();
    while (millis() - s < 10) { }
  }
}

void taskL(void* pv) {
  Serial.printf("[%5lu ms] L: taking lock\n", (unsigned long)(millis() - t0));
  xSemaphoreTake(lockHandle, portMAX_DELAY);
  for (int i = 1; i <= 3; i++) {
    busyWorkMs(100);
    Serial.printf("[%5lu ms] L: critical section %d/3, my priority = %u\n",
                  (unsigned long)(millis() - t0), i, (unsigned)uxTaskPriorityGet(NULL));
  }
  xSemaphoreGive(lockHandle);
  vTaskDelete(NULL);
}

void taskM(void* pv) {
  vTaskDelay(pdMS_TO_TICKS(150));
  Serial.printf("[%5lu ms] M: starts %d ms of work (does not use the lock)\n",
                (unsigned long)(millis() - t0), TASK_M_WORK_MS);
  busyWorkMs(TASK_M_WORK_MS);
  Serial.printf("[%5lu ms] M: finished\n", (unsigned long)(millis() - t0));
  vTaskDelete(NULL);
}

void taskH(void* pv) {
  vTaskDelay(pdMS_TO_TICKS(100));
  uint32_t requested = millis();
  Serial.printf("[%5lu ms] H: needs the lock\n", (unsigned long)(requested - t0));
  xSemaphoreTake(lockHandle, portMAX_DELAY);
  Serial.printf("[%5lu ms] H: got the lock after waiting %lu ms\n",
                (unsigned long)(millis() - t0), (unsigned long)(millis() - requested));
  busyWorkMs(50);
  xSemaphoreGive(lockHandle);
  Serial.printf("[%5lu ms] H: done\n", (unsigned long)(millis() - t0));
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
#if USE_MUTEX
  Serial.printf("\n=== Demo 3: Priority Inversion | USE_MUTEX 1 (mutex, priority inheritance) | M work %d ms ===\n", TASK_M_WORK_MS);
  lockHandle = xSemaphoreCreateMutex();   // starts "free"
#else
  Serial.printf("\n=== Demo 3: Priority Inversion | USE_MUTEX 0 (binary semaphore, NO inheritance) | M work %d ms ===\n", TASK_M_WORK_MS);
  lockHandle = xSemaphoreCreateBinary();  // starts "empty"
  xSemaphoreGive(lockHandle);             // make it available
#endif

  t0 = millis();
  xTaskCreatePinnedToCore(taskL, "Task_L", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskM, "Task_M", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskH, "Task_H", 4096, NULL, 3, NULL, 1);
}

void loop() {
  vTaskDelete(NULL); // loop() is not needed; remove its task
}
