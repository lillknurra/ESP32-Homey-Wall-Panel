#ifndef PATCH057_TEST_NVS_H
#define PATCH057_TEST_NVS_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t esp_err_t;
typedef uint32_t nvs_handle_t;

#define ESP_OK ((esp_err_t)0)
#define ESP_ERR_NVS_NOT_FOUND ((esp_err_t)0x1102)
#define ESP_ERR_NVS_INVALID_LENGTH ((esp_err_t)0x110c)
#define NVS_READONLY 0
#define NVS_READWRITE 1

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_get_blob(
    nvs_handle_t handle,
    const char *key,
    void *out_value,
    size_t *length);
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value);
esp_err_t nvs_set_blob(
    nvs_handle_t handle,
    const char *key,
    const void *value,
    size_t length);
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value);
esp_err_t nvs_erase_all(nvs_handle_t handle);
esp_err_t nvs_commit(nvs_handle_t handle);

#endif
