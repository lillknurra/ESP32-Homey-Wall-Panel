#pragma once
static inline void dummy_log(const char *fmt, ...) {(void)fmt;}
#define ESP_LOGE(tag, ...) do {(void)(tag);dummy_log(__VA_ARGS__);}while(0)
#define ESP_LOGI(tag, ...) do {(void)(tag);dummy_log(__VA_ARGS__);}while(0)
