#include "athom_cloud_model.h"
#include "panel_homey_dashboard_binding.h"
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

const char *athom_alias_activation_classification(
    bool attempted,
    panel_homey_alias_store_result_t result)
{
    if (!attempted) return "not_attempted";
    switch (result) {
    case PANEL_HOMEY_ALIAS_STORE_OK: return "ok";
    case PANEL_HOMEY_ALIAS_STORE_NOT_FOUND: return "not_found";
    case PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED: return "not_configured";
    case PANEL_HOMEY_ALIAS_STORE_INVALID: return "invalid";
    case PANEL_HOMEY_ALIAS_STORE_IO_ERROR: return "io_error";
    case PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR: return "verify_error";
    default: return "invalid";
    }
}

static const char *snapshot_publish_result_name(
    const panel_homey_snapshot_publish_inspection_t *inspection)
{
    if (inspection == NULL || !inspection->attempted) return "not_attempted";
    switch (inspection->result) {
    case PANEL_HOMEY_READ_OK: return "ok";
    case PANEL_HOMEY_READ_NOT_CONFIGURED: return "not_configured";
    case PANEL_HOMEY_READ_NOT_FOUND: return "not_found";
    case PANEL_HOMEY_READ_DUPLICATE: return "duplicate";
    case PANEL_HOMEY_READ_OVERFLOW: return "overflow";
    case PANEL_HOMEY_READ_INVALID: return "invalid";
    case PANEL_HOMEY_READ_STALE:
    default: return "invalid";
    }
}

static const char *match_value_json(panel_homey_match_value_t value)
{
    switch (value) {
    case PANEL_HOMEY_MATCH_VALUE_FALSE: return "false";
    case PANEL_HOMEY_MATCH_VALUE_TRUE: return "true";
    case PANEL_HOMEY_MATCH_VALUE_UNKNOWN:
    default: return "null";
    }
}

static const char *alias_diag_value_json(
    panel_homey_alias_diag_value_t value)
{
    switch (value) {
    case PANEL_HOMEY_ALIAS_DIAG_FALSE: return "false";
    case PANEL_HOMEY_ALIAS_DIAG_TRUE: return "true";
    case PANEL_HOMEY_ALIAS_DIAG_UNKNOWN:
    default: return "null";
    }
}

static const char *alias_store_result_json(
    panel_homey_alias_store_result_t result)
{
    switch (result) {
    case PANEL_HOMEY_ALIAS_STORE_OK: return "ok";
    case PANEL_HOMEY_ALIAS_STORE_NOT_FOUND: return "not_found";
    case PANEL_HOMEY_ALIAS_STORE_IO_ERROR: return "io_error";
    case PANEL_HOMEY_ALIAS_STORE_INVALID: return "invalid";
    case PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED: return "not_configured";
    case PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR: return "verify_error";
    case PANEL_HOMEY_ALIAS_STORE_UNSTABLE: return "unstable";
    default: return "invalid";
    }
}

static const char *alias_hint_json(panel_homey_alias_hint_t hint)
{
    switch (hint) {
    case PANEL_HOMEY_ALIAS_HINT_NONE: return "none";
    case PANEL_HOMEY_ALIAS_HINT_A: return "a";
    case PANEL_HOMEY_ALIAS_HINT_B: return "b";
    case PANEL_HOMEY_ALIAS_HINT_INVALID: return "invalid";
    case PANEL_HOMEY_ALIAS_HINT_UNKNOWN:
    default: return "unknown";
    }
}

static const char *alias_selection_basis_json(
    panel_homey_alias_selection_basis_t basis)
{
    switch (basis) {
    case PANEL_HOMEY_ALIAS_SELECTION_NONE: return "none";
    case PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT: return "active_hint";
    case PANEL_HOMEY_ALIAS_SELECTION_ONLY_VALID_SLOT: return "only_valid_slot";
    case PANEL_HOMEY_ALIAS_SELECTION_GENERATION: return "generation";
    case PANEL_HOMEY_ALIAS_SELECTION_UNKNOWN:
    default: return "unknown";
    }
}

static const char *alias_slot_json(panel_homey_alias_slot_t slot)
{
    switch (slot) {
    case PANEL_HOMEY_ALIAS_SLOT_A: return "a";
    case PANEL_HOMEY_ALIAS_SLOT_B: return "b";
    case PANEL_HOMEY_ALIAS_SLOT_NONE: return "none";
    default: return "unknown";
    }
}

static bool append_alias_slot_diagnostic(
    char *out,
    size_t capacity,
    size_t *used,
    bool first,
    const char *name,
    const panel_homey_alias_slot_diagnostic_t *slot,
    bool observation_available)
{
    const char *present = observation_available
        ? (slot->present ? "true" : "false")
        : "null";
    const char *structurally_valid = observation_available
        ? (slot->structurally_valid ? "true" : "false")
        : "null";
    const bool details_available = observation_available &&
        slot->structurally_valid;
    int written = snprintf(
        out + *used,
        capacity - *used,
        "%s\"%s\":{\"present\":%s,\"structurally_valid\":%s,\"generation\":",
        first ? "" : ",",
        name,
        present,
        structurally_valid);
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;

    if (details_available && slot->generation_known) {
        written = snprintf(out + *used, capacity - *used, "%u",
            (unsigned)slot->generation);
    } else {
        written = snprintf(out + *used, capacity - *used, "null");
    }
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;

    if (details_available && slot->entry_count_known) {
        written = snprintf(
            out + *used,
            capacity - *used,
            ",\"entry_count\":%u,\"selected_homey_match\":%s",
            (unsigned)slot->entry_count,
            alias_diag_value_json(slot->selected_homey_match));
    } else {
        written = snprintf(
            out + *used,
            capacity - *used,
            ",\"entry_count\":null,\"selected_homey_match\":null");
    }
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;

    static const char *const awning_fields[] = {
        "awning_1_present", "awning_2_present", "awning_3_present"};
    for (size_t i = 0U; i < 3U; ++i) {
        written = snprintf(
            out + *used,
            capacity - *used,
            ",\"%s\":%s",
            awning_fields[i],
            details_available
                ? alias_diag_value_json(slot->awning_present[i])
                : "null");
        if (written <= 0 || (size_t)written >= capacity - *used) return false;
        *used += (size_t)written;
    }

    written = snprintf(out + *used, capacity - *used, "}");
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;
    return true;
}

static bool append_alias_store_diagnostic(
    char *out,
    size_t capacity,
    size_t *used,
    const panel_homey_alias_store_diagnostic_t *diagnostic)
{
    const bool observation_available =
        diagnostic->observation_stable == PANEL_HOMEY_ALIAS_DIAG_TRUE &&
        (diagnostic->result == PANEL_HOMEY_ALIAS_STORE_OK ||
         diagnostic->result == PANEL_HOMEY_ALIAS_STORE_NOT_FOUND);
    int written = snprintf(
        out + *used,
        capacity - *used,
        ",\"alias_store\":{\"result\":\"%s\",\"observation_stable\":%s,\"active_slot_hint\":\"%s\","
        "\"effective_selected_slot\":\"%s\",\"selection_basis\":\"%s\",\"slots\":{",
        alias_store_result_json(diagnostic->result),
        alias_diag_value_json(diagnostic->observation_stable),
        alias_hint_json(diagnostic->active_slot_hint),
        observation_available
            ? alias_slot_json(diagnostic->effective_selected_slot)
            : "unknown",
        alias_selection_basis_json(diagnostic->selection_basis));
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;

    if (!append_alias_slot_diagnostic(
            out, capacity, used, true, "a", &diagnostic->slots[0],
            observation_available) ||
        !append_alias_slot_diagnostic(
            out, capacity, used, false, "b", &diagnostic->slots[1],
            observation_available)) {
        return false;
    }
    written = snprintf(out + *used, capacity - *used, "}}}");
    if (written <= 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;
    return true;
}

bool athom_homey_awning_snapshot_json(
    char *out,
    size_t capacity,
    const panel_homey_snapshot_inspection_t *inspection,
    const panel_homey_snapshot_publish_inspection_t *publish_inspection,
    bool alias_activation_attempted,
    panel_homey_alias_store_result_t alias_activation_result,
    const panel_homey_alias_store_diagnostic_t *alias_store_diagnostic)
{
    if (out == NULL || capacity == 0U || inspection == NULL ||
        publish_inspection == NULL || alias_store_diagnostic == NULL) return false;
    panel_homey_awning_diagnostic_t awnings[3] = {{0}};
    if (inspection->present && !panel_homey_dashboard_awning_diagnostics(
            &inspection->snapshot, awnings)) return false;
    const char *result = "invalid";
    if (inspection->result == PANEL_HOMEY_READ_OK) result = "ok";
    else if (inspection->result == PANEL_HOMEY_READ_NOT_FOUND) result = "not_found";
    else if (inspection->result == PANEL_HOMEY_READ_STALE) result = "stale";
    else if (inspection->result == PANEL_HOMEY_READ_NOT_CONFIGURED) result = "not_configured";
    int written = snprintf(
        out, capacity,
        "{\"present\":%s,\"result\":\"%s\",\"fresh\":%s,"
        "\"generation_valid\":%s,\"generation\":%u,\"age_ms\":%llu,\"item_count\":%u,"
        "\"alias_activation\":\"%s\",\"last_publish\":{"
        "\"attempted\":%s,\"result\":\"%s\",\"age_ms\":",
        inspection->present ? "true" : "false", result,
        inspection->present && inspection->fresh ? "true" : "false",
        inspection->present ? "true" : "false",
        inspection->present ? (unsigned)inspection->snapshot.generation : 0U,
        (unsigned long long)(inspection->present ? inspection->age_ms : 0ULL),
        inspection->present ? (unsigned)inspection->snapshot.item_count : 0U,
        athom_alias_activation_classification(
            alias_activation_attempted, alias_activation_result),
        publish_inspection->attempted ? "true" : "false",
        snapshot_publish_result_name(publish_inspection));
    if (written <= 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (publish_inspection->attempted && publish_inspection->age_valid) {
        written = snprintf(out + used, capacity - used, "%llu",
            (unsigned long long)publish_inspection->age_ms);
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
    } else {
        written = snprintf(out + used, capacity - used, "null");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
    }
    written = snprintf(out + used, capacity - used, "},\"awnings\":[");
    if (written <= 0 || (size_t)written >= capacity - used) return false;
    used += (size_t)written;
    static const char *const aliases[] = {"awning_1", "awning_2", "awning_3"};
    for (size_t i = 0U; i < 3U; ++i) {
        const panel_homey_awning_match_stages_t *stages =
            inspection->present
                ? &inspection->snapshot.awning_match_stages[i]
                : NULL;
        const char *matched = awnings[i].matched ? "true" : "false";
        if (stages != NULL &&
            stages->matched != PANEL_HOMEY_MATCH_VALUE_UNKNOWN) {
            matched = match_value_json(stages->matched);
        }
        written = snprintf(
            out + used, capacity - used,
            "%s{\"alias\":\"%s\",\"matched\":%s,\"available\":%s,"
            "\"binding_entry_present\":%s,\"device_present\":%s,"
            "\"capability_present\":%s}",
            i == 0U ? "" : ",", aliases[i],
            matched,
            awnings[i].matched ? (awnings[i].available ? "true" : "false") : "null",
            stages != NULL ? match_value_json(stages->binding_entry_present) : "null",
            stages != NULL ? match_value_json(stages->device_present) : "null",
            stages != NULL ? match_value_json(stages->capability_present) : "null");
        if (written <= 0 || (size_t)written >= capacity - used) return false;
        used += (size_t)written;
    }
    written = snprintf(out + used, capacity - used, "]");
    if (written <= 0 || (size_t)written >= capacity - used) return false;
    used += (size_t)written;
    return append_alias_store_diagnostic(
        out,
        capacity,
        &used,
        alias_store_diagnostic);
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

/* No private Homey record is accepted by the diagnostic serializer. */
bool athom_homey_diagnostic_status_json(
    char *out,
    size_t capacity,
    const char *state,
    size_t zone_count,
    size_t device_count)
{
    if (out == NULL || capacity == 0U) return false;
    static const char *const allowed_states[] = {
        "idle", "token_exchange", "fetching_homeys", "homey_selection_required",
        "oauth_error", "awaiting_callback", "ready", "homey_connection_error",
        "connecting_homey", "refreshing", "login_required",
        "restoring_preselection", "restoring_session",
    };
    const char *safe_state = "unknown";
    for (size_t i = 0U; state != NULL &&
         i < sizeof(allowed_states) / sizeof(allowed_states[0]); ++i) {
        if (strcmp(state, allowed_states[i]) == 0) {
            safe_state = allowed_states[i];
            break;
        }
    }
    const int written = snprintf(
        out, capacity,
        "{\"state\":\"%s\",\"zone_count\":%u,\"device_count\":%u}",
        safe_state, (unsigned)zone_count, (unsigned)device_count);
    return written > 0 && (size_t)written < capacity;
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
        written = snprintf(out + used, capacity - used, "{\"name\":");
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
        written = snprintf(out + used, capacity - used, "{\"name\":");
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
