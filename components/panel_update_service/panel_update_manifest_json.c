/* Patch116I: JSON schema v1 reader using the project's pinned cJSON component. */
#include "panel_update_manifest.h"
#include "cJSON.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool object_exact_keys(const cJSON *obj, const char *const *keys, size_t key_count)
{
    if (!cJSON_IsObject(obj)) return false;
    size_t count = 0U;
    for (const cJSON *item = obj->child; item; item = item->next) {
        bool known = false;
        if (!item->string) return false;
        for (size_t i = 0; i < key_count; i++) {
            if (strcmp(item->string, keys[i]) == 0) known = true;
        }
        if (!known) return false;
        for (const cJSON *next = item->next; next; next = next->next) {
            if (next->string && strcmp(item->string, next->string) == 0) return false;
        }
        count++;
    }
    return count == key_count;
}

static bool read_string(const cJSON *obj, const char *name, char *out, size_t capacity)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(obj, name);
    if (!cJSON_IsString(value) || !value->valuestring || !capacity) return false;
    const size_t length = strlen(value->valuestring);
    if (length >= capacity) return false;
    memcpy(out, value->valuestring, length + 1U);
    return true;
}

static bool read_u32(const cJSON *obj, const char *name, uint32_t *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, name);
    if (!cJSON_IsNumber(item) || !value) return false;
    const double d = item->valuedouble;
    if (!isfinite(d) || d < 0.0 || d > 4294967295.0 || floor(d) != d) return false;
    *value = (uint32_t)d;
    return true;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; /* Only canonical lowercase SHA-256 strings accepted. */
}

static bool read_sha256(const cJSON *object, uint8_t out[32])
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, "sha256");
    if (!cJSON_IsString(item) || !item->valuestring || strlen(item->valuestring) != 64U) return false;
    for (size_t i = 0; i < 32U; i++) {
        int hi = hex_digit(item->valuestring[i * 2U]);
        int lo = hex_digit(item->valuestring[i * 2U + 1U]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

bool panel_update_manifest_parse(const char *json, size_t json_length,
                                 panel_update_manifest_t *out)
{
    if (!json || !out || json_length == 0U ||
        json_length > PANEL_UPDATE_MANIFEST_MAX_BYTES ||
        memchr(json, '\0', json_length) != NULL) return false;

    /* Bounded copy: cJSON requires a NUL-terminated input. */
    char buffer[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
    memcpy(buffer, json, json_length);
    buffer[json_length] = '\0';
    /* Reject escaped Unicode NUL: cJSON exposes it as a shortened C string. */
    for (size_t i = 0; i + 5U < json_length; i++) {
        if (buffer[i] == '\\' && buffer[i + 1U] == 'u' &&
            memcmp(buffer + i + 2U, "0000", 4U) == 0) return false;
    }
    const char *end = NULL;
    cJSON *root = cJSON_ParseWithOpts(buffer, &end, true);
    if (!root) return false;

    const char *const top_keys[] = {"schema_version", "hardware_model", "project",
                                    "channel", "release_sequence", "version", "image", "notes"};
    const char *const image_keys[] = {"size_bytes", "sha256", "url"};
    panel_update_manifest_t parsed = {0};
    const cJSON *image = cJSON_GetObjectItemCaseSensitive(root, "image");
    bool ok = object_exact_keys(root, top_keys, sizeof(top_keys) / sizeof(top_keys[0])) &&
              object_exact_keys(image, image_keys, sizeof(image_keys) / sizeof(image_keys[0])) &&
              read_u32(root, "schema_version", &parsed.schema_version) &&
              read_string(root, "hardware_model", parsed.hardware_model, sizeof(parsed.hardware_model)) &&
              read_string(root, "project", parsed.project, sizeof(parsed.project)) &&
              read_string(root, "channel", parsed.channel, sizeof(parsed.channel)) &&
              read_u32(root, "release_sequence", &parsed.release_sequence) &&
              read_string(root, "version", parsed.version, sizeof(parsed.version)) &&
              read_string(root, "notes", parsed.notes, sizeof(parsed.notes)) &&
              read_u32(image, "size_bytes", &parsed.image_size_bytes) &&
              read_sha256(image, parsed.image_sha256) &&
              read_string(image, "url", parsed.image_url, sizeof(parsed.image_url));
    if (ok) ok = panel_update_manifest_metadata_valid(&parsed);
    cJSON_Delete(root);
    if (!ok) return false;
    *out = parsed;
    return true;
}
