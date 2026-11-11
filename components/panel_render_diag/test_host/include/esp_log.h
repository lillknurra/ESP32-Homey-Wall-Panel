#pragma once
void panel_diag_test_log(const char *tag, const char *format, ...);
#define ESP_LOGI(tag, ...) panel_diag_test_log((tag), __VA_ARGS__)
#define ESP_LOGE(tag, ...) panel_diag_test_log((tag), __VA_ARGS__)
