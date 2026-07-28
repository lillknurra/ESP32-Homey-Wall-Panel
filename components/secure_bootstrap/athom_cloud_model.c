#include "athom_cloud_model.h"
#include <stdio.h>
#include <string.h>

bool athom_token_set_apply_refresh(
    athom_token_set_t *current,
    const char *new_access_token,
    const char *new_refresh_token,
    uint32_t expires_in_s)
{
    if (current == NULL || new_access_token == NULL ||
        new_access_token[0] == '\0') {
        return false;
    }

    size_t access_length = strlen(new_access_token);
    if (access_length == 0U ||
        access_length >= sizeof(current->access_token)) {
        return false;
    }

    bool has_new_refresh =
        new_refresh_token != NULL && new_refresh_token[0] != '\0';
    size_t refresh_length = 0U;

    if (has_new_refresh) {
        refresh_length = strlen(new_refresh_token);
        if (refresh_length >= sizeof(current->refresh_token)) {
            return false;
        }
    } else if (current->refresh_token[0] == '\0') {
        return false;
    }

    memcpy(current->access_token,
           new_access_token,
           access_length + 1U);

    if (has_new_refresh) {
        memcpy(current->refresh_token,
               new_refresh_token,
               refresh_length + 1U);
    }

    current->expires_in_s = expires_in_s;
    return true;
}

const char *athom_homey_preferred_url(const athom_homey_t *homey)
{
    if (homey == NULL) return NULL;
    if (homey->local_url_secure[0] != '\0') return homey->local_url_secure;
    if (homey->local_url[0] != '\0') return homey->local_url;
    if (homey->remote_url[0] != '\0') return homey->remote_url;
    return NULL;
}

const athom_homey_t *athom_homey_find_exact(
    const athom_homey_list_t *list,
    const char *homey_id)
{
    if (list == NULL || homey_id == NULL || homey_id[0] == '\0') return NULL;
    for (size_t index = 0U; index < list->count; ++index) {
        if (strcmp(list->items[index].id, homey_id) == 0) {
            return &list->items[index];
        }
    }
    return NULL;
}

static bool append_json_string(
    char *out,
    size_t capacity,
    size_t *used,
    const char *value)
{
    if (out == NULL || used == NULL || value == NULL) return false;
    if (*used + 2U >= capacity) return false;
    out[(*used)++] = '"';
    for (size_t i = 0U; value[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)value[i];
        const char *escape = NULL;
        if (c == '"' || c == '\\') escape = c == '"' ? "\\\"" : "\\\\";
        if (escape != NULL) {
            if (*used + 2U >= capacity) return false;
            out[(*used)++] = escape[0];
            out[(*used)++] = escape[1];
        } else if (c >= 0x20U) {
            if (*used + 1U >= capacity) return false;
            out[(*used)++] = (char)c;
        }
    }
    if (*used + 1U >= capacity) return false;
    out[(*used)++] = '"';
    out[*used] = '\0';
    return true;
}

bool athom_homey_status_json(
    char *out,
    size_t capacity,
    const char *state,
    const athom_homey_list_t *homeys,
    const athom_homey_t *selected,
    size_t zone_count,
    size_t device_count)
{
    if (out == NULL || capacity == 0U || state == NULL || homeys == NULL) {
        return false;
    }

    int written = snprintf(
        out, capacity,
        "{\"state\":\"%s\",\"zone_count\":%u,\"device_count\":%u,"
        "\"selected_homey\":",
        state, (unsigned)zone_count, (unsigned)device_count);
    if (written <= 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;

    if (selected == NULL) {
        written = snprintf(out + used, capacity - used, "null,\"homeys\":[");
    } else {
        written = snprintf(out + used, capacity - used, "{\"id\":");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
        if (!append_json_string(out, capacity, &used, selected->id)) return false;
        written = snprintf(out + used, capacity - used, ",\"name\":");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
        if (!append_json_string(out, capacity, &used, selected->name)) return false;
        written = snprintf(out + used, capacity - used, "},\"homeys\":[");
    }
    if (written <= 0 || (size_t)written >= capacity - used) return false;
    used += (size_t)written;

    for (size_t i = 0U; i < homeys->count; ++i) {
        if (i != 0U) {
            if (used + 1U >= capacity) return false;
            out[used++] = ',';
            out[used] = '\0';
        }
        written = snprintf(out + used, capacity - used, "{\"id\":");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
        if (!append_json_string(out, capacity, &used, homeys->items[i].id)) return false;
        written = snprintf(out + used, capacity - used, ",\"name\":");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
        if (!append_json_string(out, capacity, &used, homeys->items[i].name)) return false;
        written = snprintf(out + used, capacity - used, "}");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
    }

    written = snprintf(out + used, capacity - used, "]}");
    return written > 0 && (size_t)written < capacity - used;
}
