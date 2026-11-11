#pragma once
#include <stddef.h>
#include <stdint.h>
typedef uint32_t StackType_t;
typedef struct { uintptr_t opaque[8]; } StaticTask_t;
typedef void *TaskHandle_t;
typedef unsigned UBaseType_t;
typedef int BaseType_t;
typedef void (*TaskFunction_t)(void *);
TaskHandle_t xTaskCreateStatic(TaskFunction_t task, const char *name,
    uint32_t stack_depth, void *arg, UBaseType_t priority, StackType_t *stack,
    StaticTask_t *control);
void vTaskDelay(uint32_t ticks);
void vTaskSuspend(TaskHandle_t task);
