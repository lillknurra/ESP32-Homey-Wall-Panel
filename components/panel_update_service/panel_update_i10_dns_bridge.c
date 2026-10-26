/* Patch116I10: bounded async DNS bridge, intentionally not linked in firmware.
 * Scheduler and DNS tickets that may still be referenced are static and are
 * never reused. Request slots are generation-fenced and can be released on
 * cancellation without waiting for a DNS callback. No TLS/HTTP is performed.
 */
#include "panel_update_i10_dns_bridge.h"

#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcpip.h"
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
#define I10_LOCK() portENTER_CRITICAL(&s_lock)
#define I10_UNLOCK() portEXIT_CRITICAL(&s_lock)
#else
#include "panel_update_i10_dns_test_api.h"
#include <stdatomic.h>
static atomic_flag s_lock = ATOMIC_FLAG_INIT;
static void i10_lock(void)
{
    while (atomic_flag_test_and_set_explicit(&s_lock, memory_order_acquire)) { }
}
static void i10_unlock(void)
{
    atomic_flag_clear_explicit(&s_lock, memory_order_release);
}
#define I10_LOCK() i10_lock()
#define I10_UNLOCK() i10_unlock()
#endif

typedef enum {
    SLOT_FREE = 0,
    SLOT_QUEUED,
    SLOT_STARTING,
    SLOT_DNS_PENDING,
    SLOT_RESULT_IPV4,
    SLOT_RESULT_ERROR,
} slot_state_t;

typedef enum {
    TICKET_FREE = 0,
    TICKET_RESERVED,
    TICKET_RETIRED,
} ticket_state_t;

typedef enum {
    TICKET_TCPIP_START = 0,
    TICKET_DNS_CALLBACK,
} ticket_kind_t;

typedef struct {
    slot_state_t state;
    uint64_t generation;
    char ipv4[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
} request_slot_t;

/* A ticket passed to lwIP/tcpip is immutable after it becomes RETIRED.
 * Retired tickets are intentionally never recycled before reboot, even after
 * callback delivery. This makes a late/duplicate callback harmless after
 * request-slot reuse. The fixed arrays bound the cost of callbacks that never
 * arrive; exhaustion fails closed instead of allocating or retrying.
 */
typedef struct {
    ticket_state_t state;
    ticket_kind_t kind;
    uint8_t slot;
    uint64_t generation;
    bool callback_seen;
} ticket_t;

static request_slot_t s_slots[PANEL_UPDATE_I10_REQUEST_SLOT_COUNT];
static ticket_t s_scheduler_tickets[PANEL_UPDATE_I10_SCHEDULER_TICKET_COUNT];
static ticket_t s_dns_tickets[PANEL_UPDATE_I10_DNS_TICKET_COUNT];
static uint64_t s_next_generation = 1U;

static bool handle_matches(const request_slot_t *slot, panel_update_i10_handle_t h)
{
    return h.slot < PANEL_UPDATE_I10_REQUEST_SLOT_COUNT &&
           slot->state != SLOT_FREE && slot->generation == h.generation;
}

static int slot_find_free(void)
{
    for (size_t i = 0U; i < PANEL_UPDATE_I10_REQUEST_SLOT_COUNT; ++i) {
        if (s_slots[i].state == SLOT_FREE) return (int)i;
    }
    return -1;
}

static int ticket_find_free(ticket_t *tickets, size_t count)
{
    for (size_t i = 0U; i < count; ++i) {
        if (tickets[i].state == TICKET_FREE) return (int)i;
    }
    return -1;
}

static ticket_t *ticket_validate_pointer(ticket_t *candidate,
                                        ticket_t *tickets, size_t count)
{
    /* Equality comparison is defined for unrelated pointers; relational
     * pointer comparisons would not be. Never dereference an untrusted lwIP
     * callback argument until it has matched a member of the static pool. */
    for (size_t i = 0U; i < count; ++i) {
        if (candidate == &tickets[i]) return &tickets[i];
    }
    return NULL;
}

static bool ip_to_text(const ip_addr_t *addr,
                       char output[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY])
{
    if (addr == NULL || !IP_IS_V4(addr)) return false;
    const ip4_addr_t *v4 = ip_2_ip4(addr);
    const int n = snprintf(output, PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY,
                           "%u.%u.%u.%u",
                           (unsigned int)ip4_addr1(v4),
                           (unsigned int)ip4_addr2(v4),
                           (unsigned int)ip4_addr3(v4),
                           (unsigned int)ip4_addr4(v4));
    return n > 0 && (size_t)n < PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY;
}

static void publish_result(uint8_t slot_index, uint64_t generation,
                           panel_update_i10_result_t result,
                           const char *ipv4)
{
    I10_LOCK();
    if (slot_index >= PANEL_UPDATE_I10_REQUEST_SLOT_COUNT ||
        s_slots[slot_index].generation != generation ||
        (s_slots[slot_index].state != SLOT_STARTING &&
         s_slots[slot_index].state != SLOT_DNS_PENDING)) {
        I10_UNLOCK();
        return;
    }
    request_slot_t *slot = &s_slots[slot_index];
    if (result == PANEL_UPDATE_I10_RESULT_IPV4 && ipv4 != NULL) {
        memcpy(slot->ipv4, ipv4, PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY);
        slot->state = SLOT_RESULT_IPV4;
    } else {
        slot->ipv4[0] = '\0';
        slot->state = SLOT_RESULT_ERROR;
    }
    I10_UNLOCK();
}

static void dns_found(const char *name, const ip_addr_t *addr, void *arg)
{
    (void)name;
    ticket_t *ticket = (ticket_t *)arg;
    char copied_ipv4[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY] = {0};
    bool valid_ipv4;
    uint8_t slot_index;
    uint64_t generation;

    if (ticket == NULL) return;
    I10_LOCK();
    ticket = ticket_validate_pointer(ticket, s_dns_tickets,
                                     PANEL_UPDATE_I10_DNS_TICKET_COUNT);
    if (ticket == NULL ||
        ticket->state != TICKET_RETIRED ||
        ticket->kind != TICKET_DNS_CALLBACK || ticket->callback_seen) {
        I10_UNLOCK();
        return;
    }
    ticket->callback_seen = true;
    slot_index = ticket->slot;
    generation = ticket->generation;
    I10_UNLOCK();

    /* lwIP's addr pointer is callback-scoped. Copy it before returning. */
    valid_ipv4 = ip_to_text(addr, copied_ipv4);
    publish_result(slot_index, generation,
                   valid_ipv4 ? PANEL_UPDATE_I10_RESULT_IPV4 :
                                PANEL_UPDATE_I10_RESULT_DNS_FAILED,
                   valid_ipv4 ? copied_ipv4 : NULL);
}

static void tcpip_start(void *arg)
{
    ticket_t *schedule_ticket = (ticket_t *)arg;
    uint8_t slot_index;
    uint64_t generation;
    int dns_ticket_index;

    I10_LOCK();
    schedule_ticket = ticket_validate_pointer(
        schedule_ticket, s_scheduler_tickets,
        PANEL_UPDATE_I10_SCHEDULER_TICKET_COUNT);
    if (schedule_ticket == NULL ||
        schedule_ticket->state != TICKET_RESERVED ||
        schedule_ticket->kind != TICKET_TCPIP_START) {
        I10_UNLOCK();
        return;
    }
    schedule_ticket->state = TICKET_RETIRED;
    slot_index = schedule_ticket->slot;
    generation = schedule_ticket->generation;
    if (slot_index >= PANEL_UPDATE_I10_REQUEST_SLOT_COUNT ||
        s_slots[slot_index].generation != generation ||
        s_slots[slot_index].state != SLOT_QUEUED) {
        I10_UNLOCK();
        return;
    }
    s_slots[slot_index].state = SLOT_STARTING;
    dns_ticket_index = ticket_find_free(s_dns_tickets,
                                        PANEL_UPDATE_I10_DNS_TICKET_COUNT);
    if (dns_ticket_index < 0) {
        I10_UNLOCK();
        publish_result(slot_index, generation,
                       PANEL_UPDATE_I10_RESULT_DNS_FAILED, NULL);
        return;
    }
    ticket_t *dns_ticket = &s_dns_tickets[dns_ticket_index];
    dns_ticket->state = TICKET_RESERVED;
    dns_ticket->kind = TICKET_DNS_CALLBACK;
    dns_ticket->slot = slot_index;
    dns_ticket->generation = generation;
    dns_ticket->callback_seen = false;
    I10_UNLOCK();

    /* Cancellation can win after the scheduler callback starts but before
     * the raw resolver is entered. Recheck so that case does not emit DNS. */
    I10_LOCK();
    const bool request_still_current =
        s_slots[slot_index].generation == generation &&
        s_slots[slot_index].state == SLOT_STARTING;
    if (!request_still_current) memset(dns_ticket, 0, sizeof(*dns_ticket));
    I10_UNLOCK();
    if (!request_still_current) return;

    /* This function runs only in TCP/IP thread context. Raw lwIP DNS API is
     * never called by the OTA worker. No lock is held across this call.
     */
    ip_addr_t resolved = {0};
    const err_t err = dns_gethostbyname_addrtype(
        PANEL_UPDATE_I10_DNS_HOST, &resolved, dns_found, dns_ticket,
        LWIP_DNS_ADDRTYPE_IPV4);

    if (err == ERR_INPROGRESS) {
        I10_LOCK();
        /* The DNS ticket becomes immutable and is never recycled. Cancellation
         * may already have freed/reused the request slot; its generation keeps
         * the eventual callback from publishing into a later request.
         */
        dns_ticket->state = TICKET_RETIRED;
        if (slot_index < PANEL_UPDATE_I10_REQUEST_SLOT_COUNT &&
            s_slots[slot_index].generation == generation &&
            s_slots[slot_index].state == SLOT_STARTING) {
            s_slots[slot_index].state = SLOT_DNS_PENDING;
        }
        I10_UNLOCK();
        return;
    }

    I10_LOCK();
    /* ERR_OK and every immediate error guarantee that no found callback will
     * be invoked. The provisional callback ticket is therefore reusable. */
    memset(dns_ticket, 0, sizeof(*dns_ticket));
    I10_UNLOCK();

    if (err == ERR_OK) {
        char copied_ipv4[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY] = {0};
        const bool valid_ipv4 = ip_to_text(&resolved, copied_ipv4);
        publish_result(slot_index, generation,
                       valid_ipv4 ? PANEL_UPDATE_I10_RESULT_IPV4 :
                                    PANEL_UPDATE_I10_RESULT_DNS_FAILED,
                       valid_ipv4 ? copied_ipv4 : NULL);
    } else {
        publish_result(slot_index, generation,
                       PANEL_UPDATE_I10_RESULT_DNS_FAILED, NULL);
    }
}

panel_update_i10_start_result_t panel_update_i10_dns_start(
    panel_update_i10_handle_t *out_handle)
{
    int slot_index;
    int ticket_index;
    ticket_t *ticket;
    err_t err;

    if (out_handle != NULL) {
        out_handle->generation = 0U;
        out_handle->slot = UINT8_MAX;
    }
    if (out_handle == NULL) return PANEL_UPDATE_I10_START_INVALID;

    I10_LOCK();
    slot_index = slot_find_free();
    if (slot_index < 0) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_START_NO_REQUEST_SLOT;
    }
    ticket_index = ticket_find_free(s_scheduler_tickets,
                                    PANEL_UPDATE_I10_SCHEDULER_TICKET_COUNT);
    if (ticket_index < 0) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_START_NO_SCHEDULER_TICKET;
    }
    if (s_next_generation == 0U || s_next_generation == UINT64_MAX) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_START_GENERATION_EXHAUSTED;
    }
    const uint64_t generation = s_next_generation++;
    request_slot_t *slot = &s_slots[slot_index];
    memset(slot, 0, sizeof(*slot));
    slot->state = SLOT_QUEUED;
    slot->generation = generation;
    ticket = &s_scheduler_tickets[ticket_index];
    ticket->state = TICKET_RESERVED;
    ticket->kind = TICKET_TCPIP_START;
    ticket->slot = (uint8_t)slot_index;
    ticket->generation = generation;
    ticket->callback_seen = false;
    out_handle->slot = (uint8_t)slot_index;
    out_handle->generation = generation;
    I10_UNLOCK();

    /* Called from the worker's thread. ERR_MEM/other errors are returned to
     * caller immediately; this bridge never waits or retries the queueing.
     */
    err = tcpip_try_callback(tcpip_start, ticket);
    if (err != ERR_OK) {
        I10_LOCK();
        if (ticket->state == TICKET_RESERVED &&
            ticket->generation == generation) {
            memset(ticket, 0, sizeof(*ticket));
        }
        if (s_slots[slot_index].generation == generation &&
            s_slots[slot_index].state == SLOT_QUEUED) {
            memset(&s_slots[slot_index], 0, sizeof(s_slots[slot_index]));
        }
        I10_UNLOCK();
        out_handle->slot = UINT8_MAX;
        out_handle->generation = 0U;
        return PANEL_UPDATE_I10_START_SCHEDULING_FAILED;
    }
    return PANEL_UPDATE_I10_START_ACCEPTED;
}

panel_update_i10_cancel_result_t panel_update_i10_dns_cancel(
    panel_update_i10_handle_t handle)
{
    if (handle.slot >= PANEL_UPDATE_I10_REQUEST_SLOT_COUNT ||
        handle.generation == 0U) return PANEL_UPDATE_I10_CANCEL_ALREADY_STALE;
    I10_LOCK();
    request_slot_t *slot = &s_slots[handle.slot];
    if (!handle_matches(slot, handle)) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_CANCEL_ALREADY_STALE;
    }
    /* Tickets/callback contexts are static and generation-fenced. Releasing a
     * DNS-only slot never frees callback memory and does not keep a Homey
     * network lock. A late DNS callback can only discard its immutable token.
     */
    memset(slot, 0, sizeof(*slot));
    I10_UNLOCK();
    return PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE;
}

panel_update_i10_result_t panel_update_i10_dns_take_result(
    panel_update_i10_handle_t handle,
    char ipv4_out[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY])
{
    if (ipv4_out != NULL) ipv4_out[0] = '\0';
    if (ipv4_out == NULL || handle.slot >= PANEL_UPDATE_I10_REQUEST_SLOT_COUNT ||
        handle.generation == 0U) return PANEL_UPDATE_I10_RESULT_STALE;
    I10_LOCK();
    request_slot_t *slot = &s_slots[handle.slot];
    if (!handle_matches(slot, handle)) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_RESULT_STALE;
    }
    if (slot->state == SLOT_QUEUED || slot->state == SLOT_STARTING ||
        slot->state == SLOT_DNS_PENDING) {
        I10_UNLOCK();
        return PANEL_UPDATE_I10_RESULT_PENDING;
    }
    if (slot->state == SLOT_RESULT_IPV4) {
        memcpy(ipv4_out, slot->ipv4, PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY);
        memset(slot, 0, sizeof(*slot));
        I10_UNLOCK();
        return PANEL_UPDATE_I10_RESULT_IPV4;
    }
    if (slot->state == SLOT_RESULT_ERROR) {
        memset(slot, 0, sizeof(*slot));
        I10_UNLOCK();
        return PANEL_UPDATE_I10_RESULT_DNS_FAILED;
    }
    I10_UNLOCK();
    return PANEL_UPDATE_I10_RESULT_STALE;
}
