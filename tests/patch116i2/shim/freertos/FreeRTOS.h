#pragma once
#include <stdint.h>
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef struct { uint32_t placeholder; } portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {0U}
#define portMAX_DELAY UINT32_MAX
#define pdTRUE 1
#define pdPASS 1
static inline void portENTER_CRITICAL(portMUX_TYPE *m) {(void)m;}
static inline void portEXIT_CRITICAL(portMUX_TYPE *m) {(void)m;}
