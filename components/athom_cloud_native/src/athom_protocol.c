#include "athom_protocol.h"
#include "athom_redaction.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *p;
    const char *end;
} json_cursor_t;

static void skip_ws(json_cursor_t *c) {
    while (c->p < c->end && isspace((unsigned char)*c->p)) ++c->p;
}

static athom_status_t parse_string(json_cursor_t *c, char *out, size_t out_size) {
    if (!c || !out || out_size == 0) return ATHOM_ERR_ARGUMENT;
    skip_ws(c);
    if (c->p >= c->end || *c->p++ != '"') return ATHOM_ERR_JSON_MALFORMED;
    size_t used = 0;
    while (c->p < c->end) {
        unsigned char ch = (unsigned char)*c->p++;
        if (ch == '"') {
            out[used] = '\0';
            return ATHOM_OK;
        }
        if (ch == '\\') {
            if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
            ch = (unsigned char)*c->p++;
            switch (ch) {
                case '"': case '\\': case '/': break;
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                default: return ATHOM_ERR_JSON_MALFORMED; /* includes unsupported \u */
            }
        } else if (ch < 0x20) {
            return ATHOM_ERR_JSON_MALFORMED;
        }
        if (used + 1 >= out_size) return ATHOM_ERR_VALUE_TOO_LARGE;
        out[used++] = (char)ch;
    }
    return ATHOM_ERR_JSON_MALFORMED;
}

static athom_status_t skip_value(json_cursor_t *c);

static athom_status_t skip_array(json_cursor_t *c) {
    if (c->p >= c->end || *c->p++ != '[') return ATHOM_ERR_JSON_MALFORMED;
    skip_ws(c);
    if (c->p < c->end && *c->p == ']') { ++c->p; return ATHOM_OK; }
    for (;;) {
        athom_status_t s = skip_value(c);
        if (s != ATHOM_OK) return s;
        skip_ws(c);
        if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
        if (*c->p == ']') { ++c->p; return ATHOM_OK; }
        if (*c->p++ != ',') return ATHOM_ERR_JSON_MALFORMED;
        skip_ws(c);
    }
}

static athom_status_t skip_object(json_cursor_t *c) {
    if (c->p >= c->end || *c->p++ != '{') return ATHOM_ERR_JSON_MALFORMED;
    skip_ws(c);
    if (c->p < c->end && *c->p == '}') { ++c->p; return ATHOM_OK; }
    for (;;) {
        char key[96];
        athom_status_t s = parse_string(c, key, sizeof(key));
        if (s != ATHOM_OK) return s;
        skip_ws(c);
        if (c->p >= c->end || *c->p++ != ':') return ATHOM_ERR_JSON_MALFORMED;
        s = skip_value(c);
        if (s != ATHOM_OK) return s;
        skip_ws(c);
        if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
        if (*c->p == '}') { ++c->p; return ATHOM_OK; }
        if (*c->p++ != ',') return ATHOM_ERR_JSON_MALFORMED;
        skip_ws(c);
    }
}

static athom_status_t skip_number(json_cursor_t *c) {
    const char *start = c->p;
    if (c->p < c->end && *c->p == '-') ++c->p;
    if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
    if (*c->p == '0') ++c->p;
    else {
        if (!isdigit((unsigned char)*c->p)) return ATHOM_ERR_JSON_MALFORMED;
        while (c->p < c->end && isdigit((unsigned char)*c->p)) ++c->p;
    }
    if (c->p < c->end && *c->p == '.') {
        ++c->p;
        if (c->p >= c->end || !isdigit((unsigned char)*c->p)) return ATHOM_ERR_JSON_MALFORMED;
        while (c->p < c->end && isdigit((unsigned char)*c->p)) ++c->p;
    }
    if (c->p < c->end && (*c->p == 'e' || *c->p == 'E')) {
        ++c->p;
        if (c->p < c->end && (*c->p == '+' || *c->p == '-')) ++c->p;
        if (c->p >= c->end || !isdigit((unsigned char)*c->p)) return ATHOM_ERR_JSON_MALFORMED;
        while (c->p < c->end && isdigit((unsigned char)*c->p)) ++c->p;
    }
    return c->p > start ? ATHOM_OK : ATHOM_ERR_JSON_MALFORMED;
}

static athom_status_t skip_value(json_cursor_t *c) {
    skip_ws(c);
    if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
    if (*c->p == '{') return skip_object(c);
    if (*c->p == '[') return skip_array(c);
    if (*c->p == '"') {
        char sink[2];
        json_cursor_t tmp = *c;
        athom_status_t s = parse_string(&tmp, sink, sizeof(sink));
        if (s == ATHOM_ERR_VALUE_TOO_LARGE) {
            /* Skip a structurally valid string without retaining it. */
            tmp = *c; ++tmp.p;
            bool escape = false;
            while (tmp.p < tmp.end) {
                unsigned char ch = (unsigned char)*tmp.p++;
                if (escape) {
                    if (ch == 'u') return ATHOM_ERR_JSON_MALFORMED;
                    if (!strchr("\"\\/bfnrt", ch)) return ATHOM_ERR_JSON_MALFORMED;
                    escape = false;
                } else if (ch == '\\') escape = true;
                else if (ch == '"') { *c = tmp; return ATHOM_OK; }
                else if (ch < 0x20) return ATHOM_ERR_JSON_MALFORMED;
            }
            return ATHOM_ERR_JSON_MALFORMED;
        }
        if (s == ATHOM_OK) *c = tmp;
        return s;
    }
    if (*c->p == '-' || isdigit((unsigned char)*c->p)) return skip_number(c);
    static const char *literals[] = {"true", "false", "null"};
    for (size_t i = 0; i < 3; ++i) {
        size_t n = strlen(literals[i]);
        if ((size_t)(c->end - c->p) >= n && memcmp(c->p, literals[i], n) == 0) {
            c->p += n; return ATHOM_OK;
        }
    }
    return ATHOM_ERR_JSON_MALFORMED;
}

static athom_status_t finish_document(json_cursor_t *c) {
    skip_ws(c);
    return c->p == c->end ? ATHOM_OK : ATHOM_ERR_JSON_MALFORMED;
}

static athom_status_t parse_i64(json_cursor_t *c, int64_t *out) {
    if (!out) return ATHOM_ERR_ARGUMENT;
    skip_ws(c);
    bool quoted = c->p < c->end && *c->p == '"';
    if (quoted) ++c->p;
    bool negative = c->p < c->end && *c->p == '-';
    if (negative) ++c->p;
    if (c->p >= c->end || !isdigit((unsigned char)*c->p)) return ATHOM_ERR_TOKEN_RESPONSE;
    uint64_t value = 0;
    while (c->p < c->end && isdigit((unsigned char)*c->p)) {
        unsigned digit = (unsigned)(*c->p++ - '0');
        if (value > ((uint64_t)INT64_MAX + (negative ? 1U : 0U) - digit) / 10U)
            return ATHOM_ERR_VALUE_TOO_LARGE;
        value = value * 10U + digit;
    }
    if (quoted) {
        if (c->p >= c->end || *c->p++ != '"') return ATHOM_ERR_TOKEN_RESPONSE;
    }
    *out = negative ? -(int64_t)value : (int64_t)value;
    return ATHOM_OK;
}

athom_status_t athom_parse_token_json(
    const char *json, size_t length, int64_t now_epoch_s,
    athom_credentials_t *updated) {
    if (!json || !updated || length == 0) return ATHOM_ERR_ARGUMENT;
    if (length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_VALUE_TOO_LARGE;
    athom_status_t status = ATHOM_ERR_TOKEN_RESPONSE;
    char token_type[24] = {0};
    char access[ATHOM_MAX_TOKEN] = {0};
    char refresh[ATHOM_MAX_TOKEN] = {0};
    athom_credentials_t candidate = {0};
    int64_t expires_in = 0;
    bool seen_type = false, seen_access = false, seen_refresh = false, seen_expiry = false;
    json_cursor_t c = {json, json + length};
    skip_ws(&c);
    if (c.p >= c.end || *c.p++ != '{') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
    skip_ws(&c);
    if (c.p < c.end && *c.p == '}') { ++c.p; status = ATHOM_ERR_TOKEN_RESPONSE; goto cleanup; }
    for (;;) {
        char key[96] = {0};
        status = parse_string(&c, key, sizeof(key));
        if (status != ATHOM_OK) goto cleanup;
        skip_ws(&c);
        if (c.p >= c.end || *c.p++ != ':') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        if (strcmp(key, "token_type") == 0) {
            if (seen_type) { status = ATHOM_ERR_JSON_DUPLICATE_KEY; goto cleanup; }
            seen_type = true; status = parse_string(&c, token_type, sizeof(token_type));
        } else if (strcmp(key, "access_token") == 0) {
            if (seen_access) { status = ATHOM_ERR_JSON_DUPLICATE_KEY; goto cleanup; }
            seen_access = true; status = parse_string(&c, access, sizeof(access));
        } else if (strcmp(key, "refresh_token") == 0) {
            if (seen_refresh) { status = ATHOM_ERR_JSON_DUPLICATE_KEY; goto cleanup; }
            seen_refresh = true; status = parse_string(&c, refresh, sizeof(refresh));
        } else if (strcmp(key, "expires_in") == 0) {
            if (seen_expiry) { status = ATHOM_ERR_JSON_DUPLICATE_KEY; goto cleanup; }
            seen_expiry = true; status = parse_i64(&c, &expires_in);
        } else {
            status = skip_value(&c);
        }
        if (status != ATHOM_OK) goto cleanup;
        skip_ws(&c);
        if (c.p >= c.end) { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        if (*c.p == '}') { ++c.p; break; }
        if (*c.p++ != ',') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        skip_ws(&c);
    }
    status = finish_document(&c);
    if (status != ATHOM_OK) goto cleanup;
    if (!seen_type || !seen_access || !seen_refresh || !seen_expiry ||
        (strcmp(token_type, "bearer") != 0 && strcmp(token_type, "Bearer") != 0) ||
        access[0] == '\0' || refresh[0] == '\0' || expires_in < 60 || expires_in > 86400) {
        status = ATHOM_ERR_TOKEN_RESPONSE; goto cleanup;
    }
    if (now_epoch_s > INT64_MAX - expires_in) { status = ATHOM_ERR_VALUE_TOO_LARGE; goto cleanup; }
    candidate = *updated;
    snprintf(candidate.access_token, sizeof(candidate.access_token), "%s", access);
    snprintf(candidate.refresh_token, sizeof(candidate.refresh_token), "%s", refresh);
    candidate.expires_at_epoch_s = now_epoch_s + expires_in;
    *updated = candidate;
    status = ATHOM_OK;
cleanup:
    athom_secure_zero(token_type, sizeof(token_type));
    athom_secure_zero(access, sizeof(access));
    athom_secure_zero(refresh, sizeof(refresh));
    athom_secure_zero(&candidate, sizeof(candidate));
    return status;
}

static athom_status_t parse_homey_object(
    json_cursor_t *c, athom_homey_summary_t *item, athom_homey_connection_t *connection) {
    if (*c->p++ != '{') return ATHOM_ERR_JSON_MALFORMED;
    bool seen_id = false, seen_name = false, seen_url = false;
    skip_ws(c);
    if (c->p < c->end && *c->p == '}') { ++c->p; return ATHOM_ERR_RESPONSE; }
    for (;;) {
        char key[96] = {0};
        athom_status_t s = parse_string(c, key, sizeof(key));
        if (s != ATHOM_OK) return s;
        skip_ws(c);
        if (c->p >= c->end || *c->p++ != ':') return ATHOM_ERR_JSON_MALFORMED;
        if (strcmp(key, "_id") == 0) {
            if (seen_id) return ATHOM_ERR_JSON_DUPLICATE_KEY;
            seen_id = true; s = parse_string(c, item->id, sizeof(item->id));
        } else if (strcmp(key, "name") == 0) {
            if (seen_name) return ATHOM_ERR_JSON_DUPLICATE_KEY;
            seen_name = true; s = parse_string(c, item->display_name, sizeof(item->display_name));
        } else if (strcmp(key, "remoteUrl") == 0) {
            if (seen_url) return ATHOM_ERR_JSON_DUPLICATE_KEY;
            seen_url = true; s = parse_string(c, connection->remote_url, sizeof(connection->remote_url));
        } else s = skip_value(c);
        if (s != ATHOM_OK) return s;
        skip_ws(c);
        if (c->p >= c->end) return ATHOM_ERR_JSON_MALFORMED;
        if (*c->p == '}') { ++c->p; break; }
        if (*c->p++ != ',') return ATHOM_ERR_JSON_MALFORMED;
        skip_ws(c);
    }
    if (!seen_id || !seen_name || !seen_url || !item->id[0] || !item->display_name[0] || !connection->remote_url[0])
        return ATHOM_ERR_RESPONSE;
    item->online = true;
    return ATHOM_OK;
}

athom_status_t athom_parse_user_homeys_json(
    const char *json, size_t length, athom_homey_list_t *out,
    athom_homey_connection_t *connections, size_t connection_capacity) {
    if (!json || !out || !connections || length == 0) return ATHOM_ERR_ARGUMENT;
    if (length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_VALUE_TOO_LARGE;
    athom_homey_list_t parsed = {0};
    athom_homey_connection_t parsed_connections[ATHOM_MAX_HOMEYS] = {0};
    bool seen_homeys = false;
    athom_status_t status = ATHOM_ERR_RESPONSE;
    json_cursor_t c = {json, json + length};
    skip_ws(&c);
    if (c.p >= c.end || *c.p++ != '{') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
    skip_ws(&c);
    if (c.p < c.end && *c.p == '}') { ++c.p; status = ATHOM_ERR_RESPONSE; goto cleanup; }
    for (;;) {
        char key[96] = {0};
        status = parse_string(&c, key, sizeof(key));
        if (status != ATHOM_OK) goto cleanup;
        skip_ws(&c);
        if (c.p >= c.end || *c.p++ != ':') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        if (strcmp(key, "homeys") == 0) {
            if (seen_homeys) { status = ATHOM_ERR_JSON_DUPLICATE_KEY; goto cleanup; }
            seen_homeys = true;
            skip_ws(&c);
            if (c.p >= c.end || *c.p++ != '[') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
            skip_ws(&c);
            if (c.p < c.end && *c.p == ']') ++c.p;
            else for (;;) {
                if (parsed.count >= ATHOM_MAX_HOMEYS || parsed.count >= connection_capacity) {
                    status = ATHOM_ERR_VALUE_TOO_LARGE; goto cleanup;
                }
                status = parse_homey_object(&c, &parsed.items[parsed.count], &parsed_connections[parsed.count]);
                if (status != ATHOM_OK) goto cleanup;
                for (size_t i = 0; i < parsed.count; ++i)
                    if (strcmp(parsed.items[i].id, parsed.items[parsed.count].id) == 0) {
                        status = ATHOM_ERR_HOMEY_DUPLICATE; goto cleanup;
                    }
                ++parsed.count;
                skip_ws(&c);
                if (c.p >= c.end) { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
                if (*c.p == ']') { ++c.p; break; }
                if (*c.p++ != ',') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
                skip_ws(&c);
            }
        } else {
            status = skip_value(&c);
            if (status != ATHOM_OK) goto cleanup;
        }
        skip_ws(&c);
        if (c.p >= c.end) { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        if (*c.p == '}') { ++c.p; break; }
        if (*c.p++ != ',') { status = ATHOM_ERR_JSON_MALFORMED; goto cleanup; }
        skip_ws(&c);
    }
    status = finish_document(&c);
    if (status != ATHOM_OK) goto cleanup;
    if (!seen_homeys) { status = ATHOM_ERR_RESPONSE; goto cleanup; }
    if (parsed.count == 0) { status = ATHOM_ERR_HOMEY_LIST_EMPTY; goto cleanup; }
    *out = parsed;
    memcpy(connections, parsed_connections, sizeof(*connections) * parsed.count);
    if (connection_capacity > parsed.count)
        memset(connections + parsed.count, 0, sizeof(*connections) * (connection_capacity - parsed.count));
    status = ATHOM_OK;
cleanup:
    athom_secure_zero(&parsed, sizeof(parsed));
    athom_secure_zero(parsed_connections, sizeof(parsed_connections));
    return status;
}

athom_status_t athom_parse_json_string(
    const char *json, size_t length, char *out, size_t out_size) {
    if (!json || !out || out_size == 0 || length == 0) return ATHOM_ERR_ARGUMENT;
    if (length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_VALUE_TOO_LARGE;
    char candidate[ATHOM_MAX_DELEGATION_TOKEN] = {0};
    if (out_size > sizeof(candidate)) return ATHOM_ERR_ARGUMENT;
    json_cursor_t c = {json, json + length};
    athom_status_t status = parse_string(&c, candidate, out_size);
    if (status == ATHOM_OK) status = finish_document(&c);
    if (status == ATHOM_OK && candidate[0] == '\0') status = ATHOM_ERR_SESSION_RESPONSE;
    if (status == ATHOM_OK) memcpy(out, candidate, strlen(candidate) + 1U);
    athom_secure_zero(candidate, sizeof(candidate));
    return status == ATHOM_ERR_JSON_MALFORMED ? ATHOM_ERR_SESSION_RESPONSE : status;
}

athom_status_t athom_count_top_level_members(
    const char *json, size_t length, size_t *out_count) {
    if (!json || !out_count || length == 0) return ATHOM_ERR_ARGUMENT;
    if (length > ATHOM_PROTOCOL_MAX_JSON_BYTES) return ATHOM_ERR_VALUE_TOO_LARGE;
    json_cursor_t c = {json, json + length};
    skip_ws(&c);
    bool object = c.p < c.end && *c.p == '{';
    bool array = c.p < c.end && *c.p == '[';
    if (!object && !array) return ATHOM_ERR_JSON_MALFORMED;
    ++c.p; skip_ws(&c);
    char close = object ? '}' : ']';
    size_t count = 0;
    if (c.p < c.end && *c.p == close) { ++c.p; if (finish_document(&c) != ATHOM_OK) return ATHOM_ERR_JSON_MALFORMED; *out_count = 0; return ATHOM_OK; }
    for (;;) {
        if (object) {
            char key[96];
            athom_status_t s = parse_string(&c, key, sizeof(key));
            if (s != ATHOM_OK) return s;
            skip_ws(&c);
            if (c.p >= c.end || *c.p++ != ':') return ATHOM_ERR_JSON_MALFORMED;
        }
        athom_status_t s = skip_value(&c);
        if (s != ATHOM_OK) return s;
        ++count; skip_ws(&c);
        if (c.p >= c.end) return ATHOM_ERR_JSON_MALFORMED;
        if (*c.p == close) { ++c.p; break; }
        if (*c.p++ != ',') return ATHOM_ERR_JSON_MALFORMED;
        skip_ws(&c);
    }
    if (finish_document(&c) != ATHOM_OK) return ATHOM_ERR_JSON_MALFORMED;
    *out_count = count;
    return ATHOM_OK;
}

athom_status_t athom_pseudonymize(const char *value, char *out, size_t out_size) {
    if (!value || !out || out_size < 20) return ATHOM_ERR_ARGUMENT;
    uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) { hash ^= *p; hash *= 1099511628211ULL; }
    snprintf(out, out_size, "id-%016llx", (unsigned long long)hash);
    return ATHOM_OK;
}

bool athom_text_is_sanitized(const char *text) {
    if (!text) return false;
    const char *forbidden[] = {"access_token", "refresh_token", "client_secret", "authorization_code", "Bearer ", "Basic ", "remoteUrl", "192.168.", "10.", ".homeypro.net"};
    for (size_t i = 0; i < sizeof(forbidden)/sizeof(forbidden[0]); ++i) if (strstr(text, forbidden[i])) return false;
    return true;
}

athom_status_t athom_json_escape_string(const char *value, char *out, size_t out_size) {
    if (!value || !out || out_size == 0) return ATHOM_ERR_ARGUMENT;
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        const char *escape = NULL; char pair[2] = {0};
        switch (*p) {
            case '"': escape = "\\\""; break;
            case '\\': escape = "\\\\"; break;
            case '\b': escape = "\\b"; break;
            case '\f': escape = "\\f"; break;
            case '\n': escape = "\\n"; break;
            case '\r': escape = "\\r"; break;
            case '\t': escape = "\\t"; break;
            default: if (*p < 0x20) return ATHOM_ERR_SESSION_RESPONSE; pair[0] = (char)*p; escape = pair; break;
        }
        size_t n = strlen(escape);
        if (used + n + 1 > out_size) return ATHOM_ERR_VALUE_TOO_LARGE;
        memcpy(out + used, escape, n); used += n;
    }
    out[used] = '\0';
    return ATHOM_OK;
}
