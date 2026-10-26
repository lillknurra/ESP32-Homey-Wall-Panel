#pragma once
/* Fake logger references all arguments and doesn't print private data. */
#include <stdarg.h>
static inline void fake_log_sink(const char *fmt, ...) { (void)fmt; }
#define ESP_LOGE(tag, ...) do { (void)(tag); fake_log_sink(__VA_ARGS__); } while (0)
#define ESP_LOGI(tag, ...) do { (void)(tag); fake_log_sink(__VA_ARGS__); } while (0)
