#pragma once
#include <pthread.h>
#include <assert.h>
typedef unsigned UBaseType_t;
typedef void *TaskHandle_t;
typedef struct { pthread_mutex_t mutex; } portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED { PTHREAD_MUTEX_INITIALIZER }
static inline void portENTER_CRITICAL(portMUX_TYPE *mux) { assert(pthread_mutex_lock(&mux->mutex) == 0); }
static inline void portEXIT_CRITICAL(portMUX_TYPE *mux) { assert(pthread_mutex_unlock(&mux->mutex) == 0); }
