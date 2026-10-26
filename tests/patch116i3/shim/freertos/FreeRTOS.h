#pragma once
#include <stdint.h>
typedef unsigned UBaseType_t;
typedef void *TaskHandle_t;
/* Host-only short critical-section ABI shim for legacy I3 adapter tests. */
typedef unsigned portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0U
static inline void portENTER_CRITICAL(portMUX_TYPE *mux) {(void)mux;}
static inline void portEXIT_CRITICAL(portMUX_TYPE *mux) {(void)mux;}
