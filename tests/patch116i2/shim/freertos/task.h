#pragma once
#include "freertos/FreeRTOS.h"
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack_bytes,
                       void *arg, unsigned priority, TaskHandle_t *task_handle);
uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t ticks_to_wait);
BaseType_t xTaskNotifyGive(TaskHandle_t task_handle);
