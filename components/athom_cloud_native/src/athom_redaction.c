#include "athom_redaction.h"
#include "athom_cloud_types.h"
#include <string.h>

void athom_secure_zero(void *buffer, size_t size) {
    volatile unsigned char *p = (volatile unsigned char *)buffer;
    while (size--) *p++ = 0;
}

bool athom_text_contains_secret(const char *text, const char *secret) {
    if (!text || !secret || secret[0] == '\0') return false;
    return strstr(text, secret) != NULL;
}

const char *athom_status_name(int status) {
    switch ((athom_status_t)status) {
        case ATHOM_OK: return "OK";
        case ATHOM_ERR_ARGUMENT: return "ARGUMENT";
        case ATHOM_ERR_STATE_MISMATCH: return "STATE_MISMATCH";
        case ATHOM_ERR_STATE_EXPIRED: return "STATE_EXPIRED";
        case ATHOM_ERR_TOKEN_EXPIRED: return "TOKEN_EXPIRED";
        case ATHOM_ERR_REFRESH_FAILED: return "REFRESH_FAILED";
        case ATHOM_ERR_HOMEY_NOT_FOUND: return "HOMEY_NOT_FOUND";
        case ATHOM_ERR_NOT_PROVISIONED: return "NOT_PROVISIONED";
        case ATHOM_ERR_STORAGE: return "STORAGE";
        case ATHOM_ERR_TRANSPORT: return "TRANSPORT";
        case ATHOM_ERR_RESPONSE: return "RESPONSE";
        case ATHOM_ERR_UNSUPPORTED: return "UNSUPPORTED";
        default: return "UNKNOWN";
    }
}
