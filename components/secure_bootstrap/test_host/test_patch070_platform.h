#pragma once
#include <pthread.h>
#include <stdint.h>
#include "http_parser.h"
typedef pthread_mutex_t portMUX_TYPE;
typedef void *TaskHandle_t;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(m) ((void)pthread_mutex_lock(m))
#define portEXIT_CRITICAL(m) ((void)pthread_mutex_unlock(m))
extern _Thread_local TaskHandle_t test_task;
extern int64_t test_clock_us;
static inline TaskHandle_t xTaskGetCurrentTaskHandle(void) { return test_task; }
static inline int64_t esp_timer_get_time(void) { return test_clock_us; }
typedef void *esp_transport_handle_t;
enum { ERR_TCP_TRANSPORT_CONNECTION_CLOSED_BY_FIN = -1,
       ERR_TCP_TRANSPORT_CONNECTION_TIMEOUT = 0 };
