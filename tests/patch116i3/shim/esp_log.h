#pragma once
void test_log(const char *level, const char *tag, const char *format, ...);
#define ESP_LOGI(...) test_log("I",__VA_ARGS__)
#define ESP_LOGW(...) test_log("W",__VA_ARGS__)
