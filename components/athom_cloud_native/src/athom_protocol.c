#include "athom_protocol.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && isspace((unsigned char)*p)) ++p;
    return p;
}

static const char *find_key(const char *json, size_t length, const char *key) {
    if (!json || !key || length == 0 || length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return NULL;
    char needle[96];
    int n = snprintf(needle, sizeof(needle), "\"%s\"", key);
    if (n <= 0 || (size_t)n >= sizeof(needle)) return NULL;
    const char *end = json + length;
    size_t needle_len = (size_t)n;
    for (const char *p = json; p + needle_len <= end; ++p) {
        if (memcmp(p, needle, needle_len) == 0) return p + needle_len;
    }
    return NULL;
}

static athom_status_t read_string_after(
    const char *json, size_t length, const char *key,
    char *out, size_t out_size, bool required) {
    const char *p = find_key(json, length, key);
    if (!p) return required ? ATHOM_ERR_RESPONSE : ATHOM_ERR_NOT_PROVISIONED;
    const char *end = json + length;
    p = skip_ws(p, end);
    if (p >= end || *p++ != ':') return ATHOM_ERR_RESPONSE;
    p = skip_ws(p, end);
    if (p + 4 <= end && memcmp(p, "null", 4) == 0) return ATHOM_ERR_NOT_PROVISIONED;
    if (p >= end || *p++ != '"') return ATHOM_ERR_RESPONSE;
    size_t used = 0;
    bool escape = false;
    while (p < end) {
        char c = *p++;
        if (escape) {
            switch (c) {
                case '"': case '\\': case '/': break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                default: return ATHOM_ERR_RESPONSE;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
            continue;
        } else if (c == '"') {
            if (used >= out_size) return ATHOM_ERR_RESPONSE;
            out[used] = '\0';
            return ATHOM_OK;
        }
        if (used + 1 >= out_size) return ATHOM_ERR_RESPONSE;
        out[used++] = c;
    }
    return ATHOM_ERR_RESPONSE;
}

static athom_status_t read_i64_after(
    const char *json, size_t length, const char *key, int64_t *out) {
    const char *p = find_key(json, length, key);
    if (!p || !out) return ATHOM_ERR_RESPONSE;
    const char *end = json + length;
    p = skip_ws(p, end);
    if (p >= end || *p++ != ':') return ATHOM_ERR_RESPONSE;
    p = skip_ws(p, end);
    bool quoted = p < end && *p == '"';
    if (quoted) ++p;
    if (p >= end || !isdigit((unsigned char)*p)) return ATHOM_ERR_RESPONSE;
    int64_t value = 0;
    while (p < end && isdigit((unsigned char)*p)) {
        int digit = *p++ - '0';
        if (value > 8640000) return ATHOM_ERR_RESPONSE;
        value = value * 10 + digit;
    }
    if (quoted && (p >= end || *p != '"')) return ATHOM_ERR_RESPONSE;
    *out = value;
    return ATHOM_OK;
}

athom_status_t athom_parse_token_json(
    const char *json, size_t length, int64_t now_epoch_s,
    athom_credentials_t *updated) {
    if (!json || !updated || length == 0 || length > ATHOM_PROTOCOL_MAX_JSON_BYTES)
        return ATHOM_ERR_ARGUMENT;
    char token_type[24] = {0};
    char access[ATHOM_MAX_TOKEN] = {0};
    char refresh[ATHOM_MAX_TOKEN] = {0};
    int64_t expires_in = 0;
    if (read_string_after(json, length, "token_type", token_type, sizeof(token_type), true) != ATHOM_OK ||
        read_string_after(json, length, "access_token", access, sizeof(access), true) != ATHOM_OK ||
        read_string_after(json, length, "refresh_token", refresh, sizeof(refresh), true) != ATHOM_OK ||
        read_i64_after(json, length, "expires_in", &expires_in) != ATHOM_OK)
        return ATHOM_ERR_RESPONSE;
    if ((strcmp(token_type, "bearer") != 0 && strcmp(token_type, "Bearer") != 0) ||
        access[0] == '\0' || refresh[0] == '\0' || expires_in < 60 || expires_in > 86400)
        return ATHOM_ERR_RESPONSE;
    snprintf(updated->access_token, sizeof(updated->access_token), "%s", access);
    snprintf(updated->refresh_token, sizeof(updated->refresh_token), "%s", refresh);
    updated->expires_at_epoch_s = now_epoch_s + expires_in;
    return ATHOM_OK;
}

static const char *find_matching(const char *start, const char *end, char open, char close) {
    int depth = 0;
    bool string = false, escape = false;
    for (const char *p = start; p < end; ++p) {
        char c = *p;
        if (string) {
            if (escape) escape = false;
            else if (c == '\\') escape = true;
            else if (c == '"') string = false;
            continue;
        }
        if (c == '"') string = true;
        else if (c == open) ++depth;
        else if (c == close && --depth == 0) return p;
    }
    return NULL;
}

athom_status_t athom_parse_user_homeys_json(
    const char *json, size_t length, athom_homey_list_t *out,
    athom_homey_connection_t *connections, size_t connection_capacity) {
    if (!json || !out || !connections || length == 0 ||
        length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_ARGUMENT;
    memset(out, 0, sizeof(*out));
    memset(connections, 0, sizeof(*connections) * connection_capacity);
    const char *p = find_key(json, length, "homeys");
    const char *end = json + length;
    if (!p) return ATHOM_ERR_RESPONSE;
    p = skip_ws(p, end);
    if (p >= end || *p++ != ':') return ATHOM_ERR_RESPONSE;
    p = skip_ws(p, end);
    if (p >= end || *p++ != '[') return ATHOM_ERR_RESPONSE;
    while (p < end) {
        p = skip_ws(p, end);
        if (p < end && *p == ']') return ATHOM_OK;
        if (p >= end || *p != '{') return ATHOM_ERR_RESPONSE;
        const char *object_end = find_matching(p, end, '{', '}');
        if (!object_end) return ATHOM_ERR_RESPONSE;
        if (out->count >= ATHOM_MAX_HOMEYS || out->count >= connection_capacity)
            return ATHOM_ERR_RESPONSE;
        size_t object_length = (size_t)(object_end - p + 1);
        athom_homey_summary_t *item = &out->items[out->count];
        athom_homey_connection_t *connection = &connections[out->count];
        if (read_string_after(p, object_length, "_id", item->id, sizeof(item->id), true) != ATHOM_OK ||
            read_string_after(p, object_length, "name", item->display_name, sizeof(item->display_name), true) != ATHOM_OK ||
            read_string_after(p, object_length, "remoteUrl", connection->remote_url,
                              sizeof(connection->remote_url), true) != ATHOM_OK)
            return ATHOM_ERR_RESPONSE;
        item->online = connection->remote_url[0] != '\0';
        ++out->count;
        p = skip_ws(object_end + 1, end);
        if (p < end && *p == ',') ++p;
    }
    return ATHOM_ERR_RESPONSE;
}

athom_status_t athom_parse_json_string(
    const char *json, size_t length, char *out, size_t out_size) {
    if (!json || !out || out_size == 0 || length == 0 ||
        length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_ARGUMENT;
    const char *p = skip_ws(json, json + length);
    const char *end = json + length;
    if (p >= end || *p++ != '"') return ATHOM_ERR_RESPONSE;
    size_t used = 0;
    bool escape = false;
    while (p < end) {
        char c = *p++;
        if (escape) {
            if (c != '"' && c != '\\' && c != '/') return ATHOM_ERR_RESPONSE;
            escape = false;
        } else if (c == '\\') {
            escape = true; continue;
        } else if (c == '"') {
            out[used] = '\0'; return used ? ATHOM_OK : ATHOM_ERR_RESPONSE;
        }
        if (used + 1 >= out_size) return ATHOM_ERR_RESPONSE;
        out[used++] = c;
    }
    return ATHOM_ERR_RESPONSE;
}

athom_status_t athom_count_top_level_members(
    const char *json, size_t length, size_t *out_count) {
    if (!json || !out_count || length == 0 || length > ATHOM_PROTOCOL_MAX_JSON_BYTES)
        return ATHOM_ERR_ARGUMENT;
    const char *p = skip_ws(json, json + length), *end = json + length;
    if (p >= end || (*p != '{' && *p != '[')) return ATHOM_ERR_RESPONSE;
    char open = *p++, close = open == '{' ? '}' : ']';
    size_t count = 0;
    int depth = 1;
    bool string = false, escape = false, has_value = false;
    for (; p < end; ++p) {
        char c = *p;
        if (string) {
            if (escape) escape = false;
            else if (c == '\\') escape = true;
            else if (c == '"') string = false;
            has_value = true; continue;
        }
        if (c == '"') { string = true; has_value = true; }
        else if (c == '{' || c == '[') { ++depth; has_value = true; }
        else if (c == '}' || c == ']') {
            if (--depth == 0) {
                if (has_value) ++count;
                if (c != close) return ATHOM_ERR_RESPONSE;
                *out_count = count; return ATHOM_OK;
            }
        } else if (c == ',' && depth == 1) {
            ++count; has_value = false;
        } else if (!isspace((unsigned char)c) && depth == 1) has_value = true;
    }
    return ATHOM_ERR_RESPONSE;
}

athom_status_t athom_pseudonymize(
    const char *value, char *out, size_t out_size) {
    if (!value || !out || out_size < 18) return ATHOM_ERR_ARGUMENT;
    uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        hash ^= *p; hash *= 1099511628211ULL;
    }
    snprintf(out, out_size, "id-%016llx", (unsigned long long)hash);
    return ATHOM_OK;
}

bool athom_text_is_sanitized(const char *text) {
    if (!text) return false;
    const char *forbidden[] = {
        "access_token", "refresh_token", "client_secret", "authorization_code",
        "Bearer ", "Basic ", "remoteUrl", "192.168.", "10.", ".homeypro.net"
    };
    for (size_t i = 0; i < sizeof(forbidden)/sizeof(forbidden[0]); ++i)
        if (strstr(text, forbidden[i])) return false;
    return true;
}
