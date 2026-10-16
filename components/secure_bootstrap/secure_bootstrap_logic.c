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

void secure_bootstrap_wifi_session_init(secure_bootstrap_wifi_session_t *session)
{
    if (session == NULL) return;
    session->generation = 0U;
    session->phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    session->cancel_from = SECURE_BOOTSTRAP_SESSION_CLOSED;
}

static bool session_generation_matches(
    const secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    return session != NULL && generation != 0U &&
        session->generation == generation;
}

uint32_t secure_bootstrap_wifi_session_begin(secure_bootstrap_wifi_session_t *session)
{
    if (session == NULL || session->phase != SECURE_BOOTSTRAP_SESSION_CLOSED) return 0U;
    session->generation++;
    if (session->generation == 0U) session->generation++;
    session->phase = SECURE_BOOTSTRAP_SESSION_OPENING;
    session->cancel_from = SECURE_BOOTSTRAP_SESSION_CLOSED;
    return session->generation;
}

bool secure_bootstrap_wifi_session_activate(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_OPENING) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_ACTIVE;
    return true;
}

bool secure_bootstrap_wifi_session_fail_open(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_OPENING) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    return true;
}

bool secure_bootstrap_wifi_session_begin_candidate(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_ACTIVE) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_CANDIDATE;
    return true;
}

bool secure_bootstrap_wifi_session_reserve_cancel(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        (session->phase != SECURE_BOOTSTRAP_SESSION_ACTIVE &&
         session->phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE)) return false;
    session->cancel_from = session->phase;
    session->phase = SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED;
    return true;
}

bool secure_bootstrap_wifi_session_cancel_start_failed(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED ||
        (session->cancel_from != SECURE_BOOTSTRAP_SESSION_ACTIVE &&
         session->cancel_from != SECURE_BOOTSTRAP_SESSION_CANDIDATE)) return false;
    session->phase = session->cancel_from;
    session->cancel_from = SECURE_BOOTSTRAP_SESSION_CLOSED;
    return true;
}

bool secure_bootstrap_wifi_session_finish_cancel(
    secure_bootstrap_wifi_session_t *session,
    uint32_t generation,
    bool keep_provisioning)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED) return false;
    if (keep_provisioning) {
        session->generation++;
        if (session->generation == 0U) session->generation++;
        session->phase = SECURE_BOOTSTRAP_SESSION_ACTIVE;
    } else {
        session->generation++;
        if (session->generation == 0U) session->generation++;
        session->phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    }
    session->cancel_from = SECURE_BOOTSTRAP_SESSION_CLOSED;
    return true;
}

bool secure_bootstrap_wifi_session_reserve_commit(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED;
    return true;
}

bool secure_bootstrap_wifi_session_commit_started(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_COMMITTING;
    return true;
}

bool secure_bootstrap_wifi_session_commit_start_failed(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED) return false;
    session->phase = SECURE_BOOTSTRAP_SESSION_CANDIDATE;
    return true;
}

bool secure_bootstrap_wifi_session_finish_commit(
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (!session_generation_matches(session, generation) ||
        session->phase != SECURE_BOOTSTRAP_SESSION_COMMITTING) return false;
    session->generation++;
    if (session->generation == 0U) session->generation++;
    session->phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    return true;
}

bool secure_bootstrap_wifi_session_is_live(
    const secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    return session_generation_matches(session, generation) &&
        (session->phase == SECURE_BOOTSTRAP_SESSION_ACTIVE ||
         session->phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);
}

bool secure_bootstrap_wifi_session_current(
    const secure_bootstrap_wifi_session_t *session,
    uint32_t *generation,
    secure_bootstrap_session_phase_t *phase)
{
    if (session == NULL || generation == NULL || phase == NULL) return false;
    *generation = session->generation;
    *phase = session->phase;
    return session->phase != SECURE_BOOTSTRAP_SESSION_CLOSED;
}

bool secure_bootstrap_sta_link_apply_event(
    bool currently_online,
    secure_bootstrap_sta_link_event_t event)
{
    if (event == SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP) return true;
    if (event == SECURE_BOOTSTRAP_STA_LINK_EVENT_DISCONNECTED) return false;
    return currently_online;
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
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL) {
            context->candidate_active = false;
            if (context->saved_config_present) {
                context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
                return SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                       SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
                       SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            }
            return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) {
            context->candidate_active = true;
            context->candidate_connect_armed = false;
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
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL) {
            context->candidate_active = false;
            context->candidate_connect_armed = false;
            if (context->saved_config_present) {
                context->retry_count = 0U;
                context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED;
                return SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                       SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
                       SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            }
            context->retry_count = 0U;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP) {
            if (!context->candidate_connect_armed) return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
            context->candidate_connect_armed = false;
            context->state = SECURE_BOOTSTRAP_WIFI_PERSISTING;
            return SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED) {
            context->retry_count++;
            if (context->retry_count < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES) {
                return SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT;
            }
            context->candidate_active = false;
            context->candidate_connect_armed = false;
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
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_TASK_CREATE_FAILED) {
            context->state = SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
            return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        }
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
            context->candidate_connect_armed = false;
            context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            return (context->saved_config_present ? SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED : SECURE_BOOTSTRAP_WIFI_ACTION_NONE) |
                   SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING;
        }
        if (event == SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_ABORTED) {
            context->candidate_active = false;
            context->candidate_connect_armed = false;
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

bool secure_bootstrap_wifi_candidate_arm_connect(
    secure_bootstrap_wifi_context_t *context,
    const secure_bootstrap_wifi_session_t *session, uint32_t generation,
    bool sta_disconnected, bool prior_events_drained)
{
    if (context == NULL || session == NULL || generation == 0U ||
        !sta_disconnected || !prior_events_drained ||
        context->state != SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE ||
        !context->candidate_active || context->candidate_connect_armed ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE ||
        session->generation != generation) return false;
    context->candidate_connect_armed = true;
    return true;
}

bool secure_bootstrap_wifi_ssid_association_matches(
    const uint8_t ap_ssid[33],
    const uint8_t active_sta_ssid[32],
    const uint8_t candidate_sta_ssid[32])
{
    if (ap_ssid == NULL || active_sta_ssid == NULL || candidate_sta_ssid == NULL ||
        memcmp(active_sta_ssid, candidate_sta_ssid, 32U) != 0) {
        return false;
    }

    size_t ap_len = 0U;
    while (ap_len < 33U && ap_ssid[ap_len] != 0U) ap_len++;
    if (ap_len == 0U || ap_len > 32U) return false;

    size_t candidate_len = 0U;
    while (candidate_len < 32U && candidate_sta_ssid[candidate_len] != 0U) {
        candidate_len++;
    }
    if (candidate_len == 0U || ap_len != candidate_len) return false;
    return memcmp(ap_ssid, candidate_sta_ssid, candidate_len) == 0;
}

uint32_t secure_bootstrap_wifi_candidate_got_ip_verified(
    secure_bootstrap_wifi_context_t *context,
    secure_bootstrap_wifi_session_t *session, uint32_t generation,
    bool current_association_verified)
{
    if (context == NULL || session == NULL || generation == 0U ||
        !current_association_verified ||
        context->state != SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE ||
        !context->candidate_active || !context->candidate_connect_armed ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE ||
        session->generation != generation ||
        !secure_bootstrap_wifi_session_reserve_commit(session, generation)) {
        return SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
    }
    return secure_bootstrap_wifi_transition(
        context, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP);
}

bool secure_bootstrap_wifi_cancel_first_install(
    secure_bootstrap_wifi_context_t *context,
    secure_bootstrap_wifi_session_t *session, uint32_t generation)
{
    if (context == NULL || session == NULL || generation == 0U ||
        context->saved_config_present ||
        session->phase != SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED ||
        session->generation != generation) return false;
    (void)secure_bootstrap_wifi_transition(
        context, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL);
    const bool reset = context->state == SECURE_BOOTSTRAP_WIFI_PROVISIONING &&
        !context->candidate_active && context->retry_count == 0U;
    return reset && secure_bootstrap_wifi_session_finish_cancel(session, generation, true);
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

bool secure_bootstrap_network_snapshot_publish(
    secure_bootstrap_network_snapshot_t *snapshot,
    const secure_bootstrap_network_t *networks, size_t count, uint32_t generation)
{
    if (snapshot == NULL || count > SECURE_BOOTSTRAP_MAX_NETWORKS || generation == 0U ||
        (count > 0U && networks == NULL)) return false;
    memset(snapshot, 0, sizeof(*snapshot));
    if (count > 0U) memcpy(snapshot->networks, networks, count * sizeof(*networks));
    snapshot->count = count;
    snapshot->session_generation = generation;
    return true;
}

bool secure_bootstrap_network_snapshot_copy(
    const secure_bootstrap_network_snapshot_t *snapshot, uint32_t expected_generation,
    secure_bootstrap_network_t *out, size_t capacity, size_t *out_count)
{
    if (snapshot == NULL || out == NULL || out_count == NULL ||
        capacity < SECURE_BOOTSTRAP_MAX_NETWORKS || expected_generation == 0U) return false;
    memset(out, 0, capacity * sizeof(*out));
    *out_count = 0U;
    if (snapshot->session_generation != expected_generation ||
        snapshot->count > SECURE_BOOTSTRAP_MAX_NETWORKS) return false;
    if (snapshot->count > 0U) {
        memcpy(out, snapshot->networks, snapshot->count * sizeof(*out));
    }
    *out_count = snapshot->count;
    return true;
}

bool secure_bootstrap_scan_begin(secure_bootstrap_scan_state_t *state, uint32_t generation)
{
    if (state == NULL || generation == 0U || state->in_progress) return false;
    state->in_progress = true;
    state->discard_result = false;
    state->session_generation = generation;
    return true;
}

void secure_bootstrap_scan_cancel(secure_bootstrap_scan_state_t *state)
{
    if (state != NULL && state->in_progress) state->discard_result = true;
}

bool secure_bootstrap_scan_complete(secure_bootstrap_scan_state_t *state,
                                    uint32_t event_generation,
                                    uint32_t active_generation, bool session_active)
{
    if (state == NULL || !state->in_progress) return false;
    const bool publish = !state->discard_result && session_active &&
        event_generation == state->session_generation &&
        active_generation == state->session_generation;
    state->in_progress = false;
    state->discard_result = false;
    state->session_generation = 0U;
    return publish;
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
