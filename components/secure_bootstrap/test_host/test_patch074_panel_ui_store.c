#include "panel_ui_store.h"
#include "nvs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool present;
    size_t size;
    uint8_t bytes[PANEL_UI_STORE_RECORD_SIZE];
} blob_t;

static blob_t slot_a;
static blob_t slot_b;
static bool active_present;
static uint8_t active_slot;
static unsigned commits;
static unsigned fail_commit_number;
static bool fail_readback;

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    assert(strcmp(name, PANEL_UI_STORE_NAMESPACE) == 0);
    assert(mode == NVS_READONLY || mode == NVS_READWRITE);
    *handle = 1U;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) { assert(handle == 1U); }

static blob_t *blob_for(const char *key)
{
    if (strcmp(key, PANEL_UI_STORE_KEY_SLOT_A) == 0) return &slot_a;
    if (strcmp(key, PANEL_UI_STORE_KEY_SLOT_B) == 0) return &slot_b;
    return NULL;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key,
                       void *out_value, size_t *length)
{
    assert(handle == 1U && length != NULL);
    blob_t *blob = blob_for(key);
    if (blob == NULL || !blob->present) return ESP_ERR_NVS_NOT_FOUND;
    if (out_value == NULL) {
        *length = blob->size;
        return ESP_OK;
    }
    if (fail_readback && out_value != NULL) {
        fail_readback = false;
        return (esp_err_t)-1;
    }
    if (*length < blob->size) return ESP_ERR_NVS_INVALID_LENGTH;
    memcpy(out_value, blob->bytes, blob->size);
    *length = blob->size;
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value)
{
    assert(handle == 1U && strcmp(key, PANEL_UI_STORE_KEY_ACTIVE) == 0);
    if (!active_present) return ESP_ERR_NVS_NOT_FOUND;
    *out_value = active_slot;
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key,
                       const void *value, size_t length)
{
    assert(handle == 1U && value != NULL && length <= PANEL_UI_STORE_RECORD_SIZE);
    blob_t *blob = blob_for(key);
    assert(blob != NULL);
    blob->present = true;
    blob->size = length;
    memcpy(blob->bytes, value, length);
    return ESP_OK;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value)
{
    assert(handle == 1U && strcmp(key, PANEL_UI_STORE_KEY_ACTIVE) == 0);
    active_present = true;
    active_slot = value;
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle)
{
    (void)handle;
    assert(0 && "settings store must never erase NVS");
    return (esp_err_t)-1;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    assert(handle == 1U);
    commits++;
    return fail_commit_number == commits ? (esp_err_t)-1 : ESP_OK;
}

static void fake_nvs_reset(void)
{
    memset(&slot_a, 0, sizeof(slot_a));
    memset(&slot_b, 0, sizeof(slot_b));
    active_present = false;
    active_slot = 0U;
    commits = 0U;
    fail_commit_number = 0U;
    fail_readback = false;
}

static void test_defaults_and_persistence(void)
{
    fake_nvs_reset();
    panel_ui_settings_t loaded;
    assert(panel_ui_store_load(&loaded) == PANEL_UI_STORE_RESULT_DEFAULTED_NOT_FOUND);
    assert(loaded.normal_brightness == PANEL_UI_DEFAULT_NORMAL_BRIGHTNESS);
    assert(commits == 0U);

    loaded.normal_brightness = 100U;
    loaded.dimmed_brightness = 50U;
    loaded.dim_after_seconds = 30U;
    loaded.off_after_seconds = 1200U;
    loaded.background_mode = PANEL_THEME_LIGHT;
    assert(panel_ui_store_save(&loaded) == PANEL_UI_STORE_RESULT_OK);
    assert(commits == 2U);
    blob_t *saved = active_slot == PANEL_UI_STORE_SLOT_A ? &slot_a : &slot_b;
    assert(saved->bytes[15] == 1U);

    panel_ui_settings_t after_restart;
    assert(panel_ui_store_load(&after_restart) == PANEL_UI_STORE_RESULT_OK);
    assert(after_restart.normal_brightness == 100U);
    assert(after_restart.dimmed_brightness == 50U);
    assert(after_restart.dim_after_seconds == 30U);
    assert(after_restart.off_after_seconds == 1200U);
    assert(after_restart.background_mode == PANEL_THEME_LIGHT);
    /* A normal app-only firmware update leaves this mocked NVS state intact. */
    assert(panel_ui_store_load(&after_restart) == PANEL_UI_STORE_RESULT_OK);
    assert(after_restart.background_mode == PANEL_THEME_LIGHT);

    loaded = after_restart;
    loaded.background_mode = PANEL_THEME_DARK;
    assert(panel_ui_store_save(&loaded) == PANEL_UI_STORE_RESULT_OK);
    saved = active_slot == PANEL_UI_STORE_SLOT_A ? &slot_a : &slot_b;
    assert(saved->bytes[15] == 0U);
    assert(panel_ui_store_load(&after_restart) == PANEL_UI_STORE_RESULT_OK);
    assert(after_restart.background_mode == PANEL_THEME_DARK);
}

static void test_corrupt_new_slot_keeps_valid_slot(void)
{
    fake_nvs_reset();
    panel_ui_settings_t first;
    panel_ui_settings_defaults(&first);
    first.normal_brightness = 60U;
    assert(panel_ui_store_save(&first) == PANEL_UI_STORE_RESULT_OK);
    panel_ui_settings_t second = first;
    second.normal_brightness = 100U;
    assert(panel_ui_store_save(&second) == PANEL_UI_STORE_RESULT_OK);
    assert(active_slot == PANEL_UI_STORE_SLOT_B);
    slot_b.bytes[slot_b.size - 1U] ^= 0x80U;

    panel_ui_settings_t loaded;
    assert(panel_ui_store_load(&loaded) == PANEL_UI_STORE_RESULT_OK);
    assert(loaded.normal_brightness == 60U);
    /* A subsequent save targets the other slot and never erases the valid one. */
    second.normal_brightness = 80U;
    assert(panel_ui_store_save(&second) == PANEL_UI_STORE_RESULT_OK);
    assert(slot_a.present && slot_b.present);
}

static void test_commit_and_readback_failures(void)
{
    fake_nvs_reset();
    panel_ui_settings_t settings;
    panel_ui_settings_defaults(&settings);
    settings.normal_brightness = 60U;
    fail_commit_number = 1U;
    assert(panel_ui_store_save(&settings) == PANEL_UI_STORE_RESULT_IO_ERROR);

    fake_nvs_reset();
    fail_readback = true;
    assert(panel_ui_store_save(&settings) == PANEL_UI_STORE_RESULT_VERIFY_ERROR);
}

int main(void)
{
    test_defaults_and_persistence();
    test_corrupt_new_slot_keeps_valid_slot();
    test_commit_and_readback_failures();
    puts("PATCH074_PANEL_UI_STORE_TESTS PASS");
    return 0;
}
