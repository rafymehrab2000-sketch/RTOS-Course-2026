/*
Real Time Systems (4_EFS8050) - Lecture 10
Demo 1 - Deadlock  (MODIFIED for Assignment 1: Deadlock)

CHANGE MADE (Part B, Step 2):
  The line that creates plan2 now makes Task_2 take mutex A first and
  mutex B second - the same order as Task_1. This removes circular wait.
  DEMO_MODE 2 is NOT used; the change is made directly in the task setup.

HOW TO RUN (Wokwi or ESP32-S3 DevKitC-1)
  Serial Monitor: 115200 baud.
  Step 1: use the ORIGINAL plan2 line (opposite order) with DEMO_MODE 0.
  Step 2: this file as it is (DEMO_MODE 0 + fixed plan2 line).
  Step 3: DEMO_MODE 1 with the ORIGINAL plan2 line (timeout + backoff).
*/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define DEMO_MODE 0

SemaphoreHandle_t mutexA;
SemaphoreHandle_t mutexB;

struct LockPlan {
  const char* taskName;
  SemaphoreHandle_t first;
  const char* firstName;
  SemaphoreHandle_t second;
  const char* secondName;
  volatile uint32_t rounds;
};

LockPlan plan1;
LockPlan plan2;

void initPlan(LockPlan &p, const char* name,
              SemaphoreHandle_t f, const char* fName,
              SemaphoreHandle_t s, const char* sName) {
  p.taskName = name;
  p.first = f; p.firstName = fName;
  p.second = s; p.secondName = sName;
  p.rounds = 0;
}

void workerTask(void* pv) {
  LockPlan* p = (LockPlan*)pv;
  for (;;) {
    Serial.printf("[%s] taking %s ...\n", p->taskName, p->firstName);
    xSemaphoreTake(p->first, portMAX_DELAY);
    Serial.printf("[%s] has %s, now needs %s\n", p->taskName, p->firstName, p->secondName);

    vTaskDelay(pdMS_TO_TICKS(100)); // simulated work: makes the bad timing certain

#if DEMO_MODE == 1
    TickType_t waitTime = pdMS_TO_TICKS(500); // finite timeout
#else
    TickType_t waitTime = portMAX_DELAY;      // wait forever
#endif

    if (xSemaphoreTake(p->second, waitTime) == pdTRUE) {
      p->rounds++;
      Serial.printf("[%s] has BOTH locks -> work done (round %lu)\n",
                    p->taskName, (unsigned long)p->rounds);
      vTaskDelay(pdMS_TO_TICKS(50));
      xSemaphoreGive(p->second); // release in reverse order
      xSemaphoreGive(p->first);
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      Serial.printf("[%s] TIMEOUT waiting for %s -> release %s, back off\n",
                    p->taskName, p->secondName, p->firstName);
      xSemaphoreGive(p->first);
      vTaskDelay(pdMS_TO_TICKS(random(100, 400))); // random backoff
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("\n=== Demo 1: Deadlock | DEMO_MODE %d | FIXED LOCK ORDER ===\n", DEMO_MODE);

  mutexA = xSemaphoreCreateMutex();
  mutexB = xSemaphoreCreateMutex();

  initPlan(plan1, "Task_1", mutexA, "A", mutexB, "B");
  // ORIGINAL (opposite order, causes deadlock):
  //   initPlan(plan2, "Task_2", mutexB, "B", mutexA, "A");
  // FIX: Task_2 now uses the same order as Task_1 (A first, then B).
  initPlan(plan2, "Task_2", mutexA, "A", mutexB, "B");

  xTaskCreatePinnedToCore(workerTask, "Task_1", 4096, &plan1, 2, NULL, 1);
  xTaskCreatePinnedToCore(workerTask, "Task_2", 4096, &plan2, 2, NULL, 1);
}

// loop() is the heartbeat: if the rounds counters stop increasing, nothing progresses.
void loop() {
  delay(1000);
  Serial.printf("[HEARTBEAT] t=%lu s | rounds: Task_1=%lu Task_2=%lu\n",
                (unsigned long)(millis() / 1000),
                (unsigned long)plan1.rounds, (unsigned long)plan2.rounds);
}
