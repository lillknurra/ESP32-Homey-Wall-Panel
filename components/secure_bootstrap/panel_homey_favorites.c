#include "panel_homey_favorites.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
static const char *TAG = "homey_favorites";
#endif

static panel_homey_favorites_public_t s_public;
#ifdef ESP_PLATFORM
static portMUX_TYPE s_public_mux = portMUX_INITIALIZER_UNLOCKED;
#define FAVORITES_LOCK() portENTER_CRITICAL(&s_public_mux)
#define FAVORITES_UNLOCK() portEXIT_CRITICAL(&s_public_mux)
#else
#define FAVORITES_LOCK() ((void)0)
#define FAVORITES_UNLOCK() ((void)0)
#endif

static const cJSON *unwrap_result(const cJSON *root)
{
    if (!cJSON_IsObject(root)) return root;
    const cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    return result != NULL ? result : root;
}

typedef struct {
    const cJSON *array;
    bool property_present;
    bool is_array;
    size_t count;
} favorite_devices_probe_t;

static favorite_devices_probe_t favorite_devices_probe(const cJSON *user_root)
{
    favorite_devices_probe_t probe = {0};
    const cJSON *user = unwrap_result(user_root);
    if (!cJSON_IsObject(user)) return probe;
    const cJSON *properties = cJSON_GetObjectItemCaseSensitive(user, "properties");
    if (!cJSON_IsObject(properties)) return probe;
    const cJSON *favorites = cJSON_GetObjectItemCaseSensitive(properties, "favoriteDevices");
    probe.property_present = favorites != NULL;
    probe.is_array = cJSON_IsArray(favorites);
    if (probe.is_array) {
        probe.array = favorites;
        probe.count = (size_t)cJSON_GetArraySize(favorites);
    }
    return probe;
}

static bool device_id_matches(const cJSON *device, const char *wanted_id)
{
    if (!cJSON_IsObject(device) || wanted_id == NULL || wanted_id[0] == '\0') return false;

    const cJSON *id = cJSON_GetObjectItemCaseSensitive(device, "id");
    if (cJSON_IsString(id) && id->valuestring != NULL && strcmp(id->valuestring, wanted_id) == 0) {
        return true;
    }

    /* Homey device collections are commonly objects keyed by device ID. */
    return device->string != NULL && strcmp(device->string, wanted_id) == 0;
}

static const cJSON *find_device(const cJSON *devices_root, const char *wanted_id)
{
    const cJSON *collection = unwrap_result(devices_root);
    if (!cJSON_IsArray(collection) && !cJSON_IsObject(collection)) return NULL;

    const cJSON *device = NULL;
    cJSON_ArrayForEach(device, collection) {
        if (device_id_matches(device, wanted_id)) return device;
    }
    return NULL;
}

static const cJSON *onoff_capability(const cJSON *device)
{
    const cJSON *capabilities = cJSON_GetObjectItemCaseSensitive(device, "capabilitiesObj");
    if (!cJSON_IsObject(capabilities)) return NULL;

    const cJSON *onoff = cJSON_GetObjectItemCaseSensitive(capabilities, "onoff");
    if (cJSON_IsObject(onoff)) return onoff;

    /* Preserve support for Homey payloads where capability objects carry id. */
    cJSON_ArrayForEach(onoff, capabilities) {
        if (!cJSON_IsObject(onoff)) continue;
        const cJSON *id = cJSON_GetObjectItemCaseSensitive(onoff, "id");
        if (cJSON_IsString(id) && id->valuestring != NULL && strcmp(id->valuestring, "onoff") == 0) {
            return onoff;
        }
    }
    return NULL;
}

static bool publish_compatible_device(
    panel_homey_favorites_public_t *target,
    const cJSON *device)
{
    if (target == NULL || target->count >= PANEL_HOMEY_FAVORITE_LIMIT || !cJSON_IsObject(device)) return false;

    const cJSON *capability = onoff_capability(device);
    if (!cJSON_IsObject(capability)) return false;

    const cJSON *value = cJSON_GetObjectItemCaseSensitive(capability, "value");
    if (!cJSON_IsBool(value)) return false;

    const cJSON *name = cJSON_GetObjectItemCaseSensitive(device, "name");
    if (!cJSON_IsString(name) || name->valuestring == NULL || name->valuestring[0] == '\0') return false;

    panel_homey_favorite_public_t *item = &target->items[target->count];
    memset(item, 0, sizeof(*item));
    const size_t name_length = strlen(name->valuestring);
    if (name_length >= sizeof(item->name)) {
        memset(item, 0, sizeof(*item));
        return false;
    }
    memcpy(item->name, name->valuestring, name_length + 1U);

    const cJSON *available = cJSON_GetObjectItemCaseSensitive(device, "available");
    item->available = !cJSON_IsFalse(available);
    item->onoff_known = true;
    item->onoff = cJSON_IsTrue(value);
    target->count++;
    return true;
}

void panel_homey_favorites_clear(void)
{
    FAVORITES_LOCK();
    memset(&s_public, 0, sizeof(s_public));
    s_public.state = PANEL_HOMEY_FAVORITES_UNVERIFIED;
    FAVORITES_UNLOCK();
}

panel_homey_favorites_state_t panel_homey_favorites_get_state(void)
{
    panel_homey_favorites_state_t state;
    FAVORITES_LOCK();
    state = s_public.state;
    FAVORITES_UNLOCK();
    return state;
}

const char *panel_homey_favorites_state_name(panel_homey_favorites_state_t state)
{
    switch (state) {
    case PANEL_HOMEY_FAVORITES_VALID_CONFIGURED:
        return "VALID_CONFIGURED";
    case PANEL_HOMEY_FAVORITES_VALID_EMPTY:
        return "VALID_EMPTY";
    case PANEL_HOMEY_FAVORITES_UNVERIFIED:
    default:
        return "UNVERIFIED";
    }
}

panel_homey_favorites_result_t panel_homey_favorites_parse_and_publish(
    const char *user_json,
    const char *devices_json)
{
    panel_homey_favorites_clear();
    if (user_json == NULL || user_json[0] == '\0' || devices_json == NULL || devices_json[0] == '\0') {
        return PANEL_HOMEY_FAVORITES_INVALID;
    }

    cJSON *user_root = cJSON_Parse(user_json);
    cJSON *devices_root = cJSON_Parse(devices_json);
    if (user_root == NULL || devices_root == NULL) {
        cJSON_Delete(user_root);
        cJSON_Delete(devices_root);
        return PANEL_HOMEY_FAVORITES_PARSE_ERROR;
    }

    const favorite_devices_probe_t favorite_probe = favorite_devices_probe(user_root);
    const cJSON *favorites = favorite_probe.array;
    const cJSON *devices = unwrap_result(devices_root);
    if (!favorite_probe.property_present || !favorite_probe.is_array ||
        (!cJSON_IsArray(devices) && !cJSON_IsObject(devices))) {
        cJSON_Delete(user_root);
        cJSON_Delete(devices_root);
        return PANEL_HOMEY_FAVORITES_PARSE_ERROR;
    }

    panel_homey_favorites_public_t next = {0};

    size_t visited = 0U;
    size_t resolved_ref_count = 0U;
    size_t compatible_ref_count = 0U;
    bool invalid_reference = favorite_probe.count > PANEL_HOMEY_FAVORITE_REFERENCE_LIMIT;
    bool unresolved_reference = false;
    bool capability_error = false;
    bool publication_error = false;
    const cJSON *favorite_id = NULL;
    if (favorites != NULL) cJSON_ArrayForEach(favorite_id, favorites) {
        if (visited++ >= PANEL_HOMEY_FAVORITE_REFERENCE_LIMIT) break;
        if (!cJSON_IsString(favorite_id) || favorite_id->valuestring == NULL || favorite_id->valuestring[0] == '\0') {
            invalid_reference = true;
            continue;
        }

        const cJSON *device = find_device(devices_root, favorite_id->valuestring);
        if (device == NULL) {
            unresolved_reference = true;
            continue;
        }
        resolved_ref_count++;

        const cJSON *capabilities = cJSON_GetObjectItemCaseSensitive(device, "capabilitiesObj");
        if (capabilities != NULL && !cJSON_IsObject(capabilities)) {
            capability_error = true;
            continue;
        }
        const cJSON *capability = onoff_capability(device);
        const cJSON *value = cJSON_IsObject(capability)
            ? cJSON_GetObjectItemCaseSensitive((cJSON *)capability, "value")
            : NULL;
        if (capability != NULL && !cJSON_IsBool(value)) {
            capability_error = true;
            continue;
        }
        const cJSON *name = cJSON_GetObjectItemCaseSensitive((cJSON *)device, "name");
        const bool compatible = cJSON_IsBool(value) && cJSON_IsString(name) &&
            name->valuestring != NULL && name->valuestring[0] != '\0';
        if (!compatible) continue;
        compatible_ref_count++;
        if (next.count < PANEL_HOMEY_FAVORITE_LIMIT) {
            if (!publish_compatible_device(&next, device)) publication_error = true;
        }
    }

    if (favorite_probe.count == 0U) {
        next.state = PANEL_HOMEY_FAVORITES_VALID_EMPTY;
    } else if (compatible_ref_count == 0U || invalid_reference || unresolved_reference ||
               capability_error || publication_error) {
        next.state = PANEL_HOMEY_FAVORITES_UNVERIFIED;
    } else {
        next.state = PANEL_HOMEY_FAVORITES_VALID_CONFIGURED;
    }

#ifdef ESP_PLATFORM
    ESP_LOGI(TAG,
        "HOMEY_FAVORITES property_present=%s is_array=%s count=%u",
        favorite_probe.property_present ? "yes" : "no",
        favorite_probe.is_array ? "yes" : "no",
        (unsigned)favorite_probe.count);
    ESP_LOGI(TAG, "HOMEY_FAVORITES resolved_ref_count=%u",
        (unsigned)resolved_ref_count);
    ESP_LOGI(TAG, "HOMEY_FAVORITES compatible_ref_count=%u",
        (unsigned)compatible_ref_count);
    ESP_LOGI(TAG, "HOMEY_FAVORITES validation_state=%s",
        panel_homey_favorites_state_name(next.state));
    for (size_t slot = 0U; slot < PANEL_HOMEY_FAVORITE_LIMIT; ++slot) {
        const panel_homey_favorite_public_t *item =
            slot < next.count ? &next.items[slot] : NULL;
        ESP_LOGI(TAG,
            "HOMEY_FAVORITES widget%u name=%s onoff_known=%s onoff_value=%s",
            (unsigned)(4U + slot),
            item != NULL ? item->name : "none",
            item != NULL && item->onoff_known ? "yes" : "no",
            item != NULL && item->onoff_known
                ? (item->onoff ? "true" : "false")
                : "unknown");
    }
#else
    /* Host builds do not emit ESP_LOGI instrumentation. Keep the counters
     * live under -Werror without changing firmware/runtime behavior. */
    (void)resolved_ref_count;
    (void)compatible_ref_count;
#endif

    cJSON_Delete(user_root);
    cJSON_Delete(devices_root);

    /* Publish one complete snapshot so UI readers can never observe a
     * partially rebuilt Favorite Devices state. */
    FAVORITES_LOCK();
    s_public = next;
    FAVORITES_UNLOCK();
    return PANEL_HOMEY_FAVORITES_OK;
}

bool panel_homey_favorites_copy_public(panel_homey_favorites_public_t *out)
{
    if (out == NULL) return false;
    FAVORITES_LOCK();
    *out = s_public;
    FAVORITES_UNLOCK();
    return true;
}

bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model)
{
    if (model == NULL) return false;
    panel_homey_favorites_public_t snapshot;
    if (!panel_homey_favorites_copy_public(&snapshot)) return false;
    bool changed = false;
    for (size_t slot = 0U; slot < PANEL_HOMEY_FAVORITE_LIMIT; ++slot) {
        const size_t widget = 4U + slot;
        const panel_homey_favorite_public_t *item =
            slot < snapshot.count ? &snapshot.items[slot] : NULL;
        const char *title = item != NULL
            ? item->name
            : (slot == 0U ? "Belysning 1" : "Belysning 2");
        panel_widget_status_t status = snapshot.state == PANEL_HOMEY_FAVORITES_UNVERIFIED
            ? PANEL_WIDGET_UNKNOWN
            : PANEL_WIDGET_UNCONFIGURED;
        bool has_boolean = false;
        bool boolean_value = false;

        if (snapshot.state == PANEL_HOMEY_FAVORITES_UNVERIFIED) {
            status = PANEL_WIDGET_UNKNOWN;
        } else if (item != NULL) {
            if (!item->available) status = PANEL_WIDGET_UNAVAILABLE;
            else if (!item->onoff_known) status = PANEL_WIDGET_UNKNOWN;
            else {
                status = PANEL_WIDGET_AVAILABLE;
                has_boolean = true;
                boolean_value = item->onoff;
            }
        }

        if (strcmp(model->widget_title[widget], title) != 0 ||
            model->widget_status[widget] != status ||
            model->widget_has_boolean[widget] != has_boolean ||
            model->widget_boolean_value[widget] != boolean_value) {
            changed = true;
        }

        (void)snprintf(
            model->widget_title[widget],
            sizeof(model->widget_title[widget]),
            "%s",
            title);
        model->widget_status[widget] = status;
        model->widget_has_boolean[widget] = has_boolean;
        model->widget_boolean_value[widget] = boolean_value;
    }
    return changed;
}
