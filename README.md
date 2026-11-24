# RTOS-Course-2026

Engineering portfolio for **Real Time Systems (4_EFS8050)**, Savonia UAS, autumn 2026.
Platform: ESP32-S3 DevKitC-1 with FreeRTOS (Arduino-ESP32), tested in the Wokwi simulator.

## Contents

| Folder | Topic | Assignment |
|---|---|---|
| `Lecture10-Deadlock/` | Deadlock, Coffman conditions, lock ordering, timeout + backoff | Assignment 1: Deadlock |
| `Lecture10-Starvation/` | Starvation, fixed-priority scheduling, blocking | Assignment 2: Starvation |
| `Lecture11-PriorityInversion/` | Priority inversion, mutex vs binary semaphore, priority inheritance | Assignment 1: Priority Inversion |
| `Lecture12-DualCore/` | Core affinity, queues, mutex-protected shared data | Dual-Core Data Acquisition and Processing |
| `Project/` | Final FreeRTOS project | Project Report, Code and Presentation |

Each folder has its own `README.md` with my answers, the Serial Monitor output from my runs, and what I learned.

## How to run

Each sketch runs in Wokwi (paste it into the course's ESP32-S3 project) or on an ESP32-S3 DevKitC-1 with Arduino IDE 2.x, board "ESP32S3 Dev Module", Serial Monitor at 115200 baud.

## Use of AI

I used an AI assistant to help structure the READMEs, explain concepts and extend the lecture code. I ran all experiments myself, the outputs in this repository are from my own runs, and I checked every explanation against my results and the lecture notes. Each folder describes how AI was used for that assignment.
