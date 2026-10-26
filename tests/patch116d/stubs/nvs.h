#pragma once
#include "esp_err.h"
#include <stdint.h>
typedef uint32_t nvs_handle_t;
#define NVS_READONLY 1
esp_err_t nvs_open(const char *namespace, int mode, nvs_handle_t *handle);
void nvs_close(nvs_handle_t handle);
