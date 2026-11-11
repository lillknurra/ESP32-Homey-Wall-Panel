#pragma once
#include <pthread.h>
#include <stdint.h>
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(mux) ((void)pthread_mutex_lock((mux)))
#define portEXIT_CRITICAL(mux) ((void)pthread_mutex_unlock((mux)))
#define portENTER_CRITICAL_ISR(mux) ((void)pthread_mutex_lock((mux)))
#define portEXIT_CRITICAL_ISR(mux) ((void)pthread_mutex_unlock((mux)))
#define portENTER_CRITICAL_SAFE(mux) ((void)pthread_mutex_lock((mux)))
#define portEXIT_CRITICAL_SAFE(mux) ((void)pthread_mutex_unlock((mux)))
#define pdMS_TO_TICKS(ms) (ms)
#define portMAX_DELAY UINT32_MAX
