#include "lwip/close_diag.h"
#include "lwip/sys.h"

#include <stdbool.h>
#include <stddef.h>

static lwip_close_diag_event_t s_events[LWIP_CLOSE_DIAG_CAPACITY];
static uint32_t s_sequence;
static uint32_t s_overwritten;
static uint32_t s_count;
static uint32_t s_next_correlation_id;
static uint32_t s_scope_overflow_count;

typedef struct {
    sys_thread_t owner;
    uint32_t correlation_id;
    uint8_t nested_suppression;
    bool active;
} close_diag_scope_t;

static close_diag_scope_t s_scopes[LWIP_CLOSE_DIAG_SCOPE_CAPACITY];

static uint32_t current_correlation_id(sys_thread_t owner)
{
    for (uint32_t i = 0U; i < LWIP_CLOSE_DIAG_SCOPE_CAPACITY; ++i) {
        if (s_scopes[i].active && s_scopes[i].owner == owner) {
            return s_scopes[i].nested_suppression == 0U
                ? s_scopes[i].correlation_id : 0U;
        }
    }
    return 0U;
}

static void record_event(
    uint8_t kind, uint8_t outcome, uint8_t flags,
    int32_t result, int32_t error_number)
{
    const uint32_t timestamp = sys_now();
    const sys_thread_t owner = xTaskGetCurrentTaskHandle();
    SYS_ARCH_DECL_PROTECT(protection_level);
    SYS_ARCH_PROTECT(protection_level);
    const uint32_t sequence = ++s_sequence;
    const uint32_t slot = (sequence - 1U) % LWIP_CLOSE_DIAG_CAPACITY;
    if (s_count < LWIP_CLOSE_DIAG_CAPACITY) {
        s_count++;
    } else {
        s_overwritten++;
    }
    s_events[slot] = (lwip_close_diag_event_t) {
        .sequence = sequence,
        .monotonic_ms = timestamp,
        .correlation_id = current_correlation_id(owner),
        .result = result,
        .error_number = error_number,
        .kind = kind,
        .outcome = outcome,
        .flags = flags,
        .reserved = 0U,
    };
    SYS_ARCH_UNPROTECT(protection_level);
}

void lwip_close_diag_socket_event(
    uint8_t outcome, uint8_t flags, int32_t result, int32_t error_number)
{
    record_event(LWIP_CLOSE_DIAG_SOCKET_CLOSE, outcome, flags, result, error_number);
}

void lwip_close_diag_transport_event(
    uint8_t kind, uint8_t outcome, uint8_t flags,
    int32_t result, int32_t error_number)
{
    if (kind != LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN &&
        kind != LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END &&
        kind != LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP &&
        kind != LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP_END &&
        kind != LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN &&
        kind != LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END) {
        return;
    }
    record_event(kind, outcome, flags, result, error_number);
}

void lwip_close_diag_snapshot(lwip_close_diag_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }
    SYS_ARCH_DECL_PROTECT(protection_level);
    SYS_ARCH_PROTECT(protection_level);
    snapshot->newest_sequence = s_sequence;
    snapshot->overwritten_count = s_overwritten;
    snapshot->scope_overflow_count = s_scope_overflow_count;
    snapshot->event_count = s_count;
    const uint32_t first_sequence = s_sequence - s_count + 1U;
    for (uint32_t i = 0U; i < s_count; ++i) {
        const uint32_t sequence = first_sequence + i;
        snapshot->events[i] = s_events[(sequence - 1U) % LWIP_CLOSE_DIAG_CAPACITY];
    }
    SYS_ARCH_UNPROTECT(protection_level);
}

uint32_t lwip_close_diag_scope_begin(void)
{
    const sys_thread_t owner = xTaskGetCurrentTaskHandle();
    if (owner == NULL) {
        return 0U;
    }

    uint32_t correlation_id = 0U;
    SYS_ARCH_DECL_PROTECT(protection_level);
    SYS_ARCH_PROTECT(protection_level);
    close_diag_scope_t *free_scope = NULL;
    for (uint32_t i = 0U; i < LWIP_CLOSE_DIAG_SCOPE_CAPACITY; ++i) {
        close_diag_scope_t *scope = &s_scopes[i];
        if (scope->active && scope->owner == owner) {
            if (scope->nested_suppression < UINT8_MAX) {
                scope->nested_suppression++;
            }
            s_scope_overflow_count++;
            SYS_ARCH_UNPROTECT(protection_level);
            return 0U;
        }
        if (!scope->active && free_scope == NULL) {
            free_scope = scope;
        }
    }
    if (free_scope == NULL) {
        s_scope_overflow_count++;
    } else {
        correlation_id = ++s_next_correlation_id;
        if (correlation_id == 0U) {
            correlation_id = ++s_next_correlation_id;
        }
        *free_scope = (close_diag_scope_t) {
            .owner = owner,
            .correlation_id = correlation_id,
            .nested_suppression = 0U,
            .active = true,
        };
    }
    SYS_ARCH_UNPROTECT(protection_level);
    return correlation_id;
}

void lwip_close_diag_scope_end(uint32_t correlation_id)
{
    const sys_thread_t owner = xTaskGetCurrentTaskHandle();
    if (owner == NULL) {
        return;
    }

    SYS_ARCH_DECL_PROTECT(protection_level);
    SYS_ARCH_PROTECT(protection_level);
    for (uint32_t i = 0U; i < LWIP_CLOSE_DIAG_SCOPE_CAPACITY; ++i) {
        close_diag_scope_t *scope = &s_scopes[i];
        if (!scope->active || scope->owner != owner) {
            continue;
        }
        if (correlation_id == 0U && scope->nested_suppression != 0U) {
            scope->nested_suppression--;
        } else if (correlation_id != 0U &&
                   scope->nested_suppression == 0U &&
                   scope->correlation_id == correlation_id) {
            *scope = (close_diag_scope_t) {0};
        }
        break;
    }
    SYS_ARCH_UNPROTECT(protection_level);
}
