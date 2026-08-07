#include "secure_bootstrap.h"

#include <string.h>

static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

secure_bootstrap_status_t secure_bootstrap_code_generate(
    secure_bootstrap_code_t *state,
    int64_t now_s,
    const uint8_t *random_bytes,
    size_t random_len)
{
    if (state == NULL || random_bytes == NULL ||
        random_len < SECURE_BOOTSTRAP_CODE_LEN || now_s < 0) {
        return SECURE_BOOTSTRAP_ERR_ARGUMENT;
    }

    secure_bootstrap_code_wipe(state);
    for (size_t i = 0U; i < SECURE_BOOTSTRAP_CODE_LEN; ++i) {
        state->code[i] = alphabet[random_bytes[i] % (sizeof(alphabet) - 1U)];
    }
    state->code[SECURE_BOOTSTRAP_CODE_LEN] = '\0';
    state->issued_at_s = now_s;
    state->consumed = false;
    return SECURE_BOOTSTRAP_OK;
}

secure_bootstrap_status_t secure_bootstrap_code_verify_and_consume(
    secure_bootstrap_code_t *state,
    const char *candidate,
    int64_t now_s)
{
    if (state == NULL || candidate == NULL || now_s < 0) {
        return SECURE_BOOTSTRAP_ERR_ARGUMENT;
    }
    if (state->consumed) {
        return SECURE_BOOTSTRAP_ERR_CONSUMED;
    }
    if (now_s < state->issued_at_s ||
        now_s - state->issued_at_s >= SECURE_BOOTSTRAP_CODE_TTL_SECONDS) {
        secure_bootstrap_code_wipe(state);
        state->consumed = true;
        return SECURE_BOOTSTRAP_ERR_EXPIRED;
    }

    unsigned diff = 0U;
    size_t candidate_len = strlen(candidate);
    diff |= (unsigned)(candidate_len ^ SECURE_BOOTSTRAP_CODE_LEN);
    for (size_t i = 0U; i < SECURE_BOOTSTRAP_CODE_LEN; ++i) {
        unsigned char value =
            i < candidate_len ? (unsigned char)candidate[i] : 0U;
        diff |= (unsigned)(value ^ (unsigned char)state->code[i]);
    }
    if (diff != 0U) {
        return SECURE_BOOTSTRAP_ERR_MISMATCH;
    }

    state->consumed = true;
    return SECURE_BOOTSTRAP_OK;
}

bool secure_bootstrap_code_rotation_due(
    const secure_bootstrap_code_t *state,
    int64_t now_s)
{
    if (state == NULL || state->code[0] == '\0' || state->consumed ||
        now_s < state->issued_at_s) {
        return false;
    }
    return now_s - state->issued_at_s >= SECURE_BOOTSTRAP_CODE_TTL_SECONDS;
}

void secure_bootstrap_code_wipe(secure_bootstrap_code_t *state)
{
    if (state == NULL) {
        return;
    }

    volatile unsigned char *p = (volatile unsigned char *)state->code;
    for (size_t i = 0U; i < sizeof(state->code); ++i) {
        p[i] = 0U;
    }
    state->issued_at_s = 0;
    state->consumed = false;
}

bool secure_bootstrap_text_contains_code(
    const char *text,
    const secure_bootstrap_code_t *state)
{
    if (text == NULL || state == NULL || state->code[0] == '\0') {
        return false;
    }
    return strstr(text, state->code) != NULL;
}

void secure_bootstrap_wipe_tracker_reset(secure_bootstrap_wipe_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
    tracker->pressed_since_ms = 0;
    tracker->triggered = false;
}

bool secure_bootstrap_wipe_tracker_update(
    secure_bootstrap_wipe_tracker_t *tracker,
    bool pressed,
    int64_t now_ms)
{
    if (tracker == NULL || now_ms < 0) {
        return false;
    }
    if (!pressed) {
        tracker->pressed_since_ms = 0;
        tracker->triggered = false;
        return false;
    }
    if (tracker->triggered) {
        return true;
    }
    if (tracker->pressed_since_ms == 0) {
        tracker->pressed_since_ms = now_ms;
        return false;
    }
    if (now_ms - tracker->pressed_since_ms >= SECURE_BOOTSTRAP_WIPE_HOLD_MS) {
        tracker->triggered = true;
    }
    return tracker->triggered;
}


void secure_bootstrap_wifi_context_init(secure_bootstrap_wifi_context_t *context)
{
    if (context == NULL) return;
    memset(context, 0, sizeof(*context));
    context->state = SECURE_BOOTSTRAP_WIFI_BOOT;
}

uint32_t secure_bootstrap_wifi_transition(
    secure_bootstrap_wifi_context_t *context,
    secure_bootstrap_wifi_event_t event)
{
    if (context == NULL) return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
    switch (context->state) {
    case SECURE_BOOTSTRAP_WIFI_BOOT:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED) {
            context->saved_config_present = true;
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) {
            context->saved_config_present = false;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP) {
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_ONLINE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                   SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED) {
            context->retry_count++;
            if (context->retry_count < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES) return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_SAVED_CONNECTION_FAILED:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_RETRY) {
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE) {
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_PROVISIONING:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) {
            context->candidate_active = true;
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED &&
            context->saved_config_present) {
            context->state = SECURE_BOOTSTRAP_WIFI_ONLINE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                   SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP || event == SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP) {
            context->state = SECURE_BOOTSTRAP_WIFI_PERSISTING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED) {
            context->retry_count++;
            if (context->retry_count < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES) {
                return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            }
            context->candidate_active = false;
            if (context->saved_config_present) {
                context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
                return SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
                       SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            }
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_PERSISTING:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK) {
            context->state = SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST_VERIFY;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED) {
            context->state = SECURE_BOOTSTRAP_WIFI_ROLLING_BACK;
            return SECURE_BOOTSTRAP_WIFI_ACTION_START_ROLLBACK;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP) {
            context->candidate_active = false;
            context->saved_config_present = true;
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_ONLINE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                   SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED ||
            event == SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_FAILED) {
            context->state = SECURE_BOOTSTRAP_WIFI_ROLLING_BACK;
            return SECURE_BOOTSTRAP_WIFI_ACTION_START_ROLLBACK;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_ROLLING_BACK:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK) {
            context->candidate_active = false;
            context->state = context->saved_config_present
                ? SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED
                : SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return context->saved_config_present
                ? SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT
                : SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED) {
            context->candidate_active = false;
            context->saved_config_present = false;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_CANDIDATE_CONNECTION_FAILED:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_RETRY) {
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE) {
            context->candidate_active = false;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return (context->saved_config_present ? SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED : SECURE_BOOTSTRAP_WIFI_ACTION_NONE) |
                   SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_ABORTED) {
            context->candidate_active = false;
            context->state = context->saved_config_present ? SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED : SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return context->saved_config_present
                ? (SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED | SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT)
                : SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    case SECURE_BOOTSTRAP_WIFI_ONLINE:
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED) {
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
            return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE) {
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        break;
    }
    return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
}


uint32_t secure_bootstrap_crc32(const void *data, size_t size)
{
    if (data == NULL && size != 0U) return 0U;
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

bool secure_bootstrap_wifi_backup_encode(
    secure_bootstrap_wifi_backup_blob_t *blob,
    const void *payload,
    size_t payload_size)
{
    if (blob == NULL || payload == NULL || payload_size == 0U ||
        payload_size > SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX) return false;
    memset(blob, 0, sizeof(*blob));
    blob->magic = SECURE_BOOTSTRAP_WIFI_BACKUP_MAGIC;
    blob->version = SECURE_BOOTSTRAP_WIFI_BACKUP_VERSION;
    blob->payload_size = (uint16_t)payload_size;
    memcpy(blob->payload, payload, payload_size);
    blob->payload_crc32 = secure_bootstrap_crc32(payload, payload_size);
    return true;
}

bool secure_bootstrap_wifi_backup_decode(
    const secure_bootstrap_wifi_backup_blob_t *blob,
    void *payload,
    size_t payload_capacity)
{
    if (blob == NULL || payload == NULL ||
        blob->magic != SECURE_BOOTSTRAP_WIFI_BACKUP_MAGIC ||
        blob->version != SECURE_BOOTSTRAP_WIFI_BACKUP_VERSION ||
        blob->payload_size == 0U ||
        blob->payload_size > SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX ||
        blob->payload_size > payload_capacity ||
        secure_bootstrap_crc32(blob->payload, blob->payload_size) != blob->payload_crc32) return false;
    memcpy(payload, blob->payload, blob->payload_size);
    return true;
}

size_t secure_bootstrap_network_insert(
    secure_bootstrap_network_t *networks,
    size_t count,
    size_t capacity,
    const char *ssid,
    int8_t rssi,
    secure_bootstrap_wifi_security_t security)
{
    if (networks == NULL || ssid == NULL || ssid[0] == '\0' || capacity == 0U) return count;
    size_t len = strlen(ssid);
    if (len > SECURE_BOOTSTRAP_SSID_MAX_LEN) return count;
    for (size_t i = 0U; i < count; ++i) {
        if (strcmp(networks[i].ssid, ssid) == 0) {
            if (rssi > networks[i].rssi) { networks[i].rssi = rssi; networks[i].security = security; }
            for (size_t j = i; j > 0U && networks[j].rssi > networks[j - 1U].rssi; --j) {
                secure_bootstrap_network_t tmp = networks[j]; networks[j] = networks[j - 1U]; networks[j - 1U] = tmp;
            }
            return count;
        }
    }
    if (count >= capacity) return count;
    memset(&networks[count], 0, sizeof(networks[count]));
    memcpy(networks[count].ssid, ssid, len + 1U);
    networks[count].rssi = rssi;
    networks[count].security = security;
    count++;
    for (size_t j = count - 1U; j > 0U && networks[j].rssi > networks[j - 1U].rssi; --j) {
        secure_bootstrap_network_t tmp = networks[j]; networks[j] = networks[j - 1U]; networks[j - 1U] = tmp;
    }
    return count;
}

const char *secure_bootstrap_signal_label(int8_t rssi)
{
    if (rssi >= -55) return "strong";
    if (rssi >= -70) return "medium";
    return "weak";
}

const char *secure_bootstrap_security_label(secure_bootstrap_wifi_security_t security)
{
    switch (security) {
    case SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN: return "Open";
    case SECURE_BOOTSTRAP_WIFI_SECURITY_WEP: return "WEP";
    case SECURE_BOOTSTRAP_WIFI_SECURITY_WPA: return "WPA";
    case SECURE_BOOTSTRAP_WIFI_SECURITY_WPA2: return "WPA2";
    case SECURE_BOOTSTRAP_WIFI_SECURITY_WPA3: return "WPA3";
    case SECURE_BOOTSTRAP_WIFI_SECURITY_ENTERPRISE: return "Enterprise";
    default: return "Unknown";
    }
}

bool secure_bootstrap_html_escape(const char *source, char *destination, size_t destination_size)
{
    if (source == NULL || destination == NULL || destination_size == 0U) return false;
    size_t used = 0U;
    for (size_t i = 0U; source[i] != '\0'; ++i) {
        const char *replacement = NULL;
        switch (source[i]) {
        case '&': replacement = "&amp;"; break;
        case '<': replacement = "&lt;"; break;
        case '>': replacement = "&gt;"; break;
        case '"': replacement = "&quot;"; break;
        case '\'': replacement = "&#39;"; break;
        default: break;
        }
        size_t need = replacement != NULL ? strlen(replacement) : 1U;
        if (used + need + 1U > destination_size) { destination[0] = '\0'; return false; }
        if (replacement != NULL) { memcpy(destination + used, replacement, need); used += need; }
        else destination[used++] = source[i];
    }
    destination[used] = '\0';
    return true;
}
