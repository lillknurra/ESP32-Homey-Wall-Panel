#include "panel_update_i10_dns_bridge.h"
#include "panel_update_i10_dns_test_api.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { tcpip_callback_fn fn; void *arg; } queued_callback_t;
typedef struct { dns_found_callback fn; void *arg; } queued_dns_t;

static queued_callback_t queue_items[64];
static size_t queue_count;
static queued_dns_t dns_items[32];
static size_t dns_count;
static err_t schedule_result;
static err_t dns_result;
static ip_addr_t immediate_address;
static unsigned dns_call_count;
static char dns_hostname[64];
static unsigned char dns_addrtype;
static bool cancel_inside_dns;
static panel_update_i10_handle_t cancel_handle;

err_t tcpip_try_callback(tcpip_callback_fn function, void *argument)
{
    if (schedule_result != ERR_OK) return schedule_result;
    assert(queue_count < sizeof(queue_items) / sizeof(queue_items[0]));
    queue_items[queue_count++] = (queued_callback_t){function, argument};
    return ERR_OK;
}

err_t dns_gethostbyname_addrtype(const char *hostname, ip_addr_t *address,
                                 dns_found_callback found, void *argument,
                                 unsigned char address_type)
{
    dns_call_count++;
    (void)snprintf(dns_hostname, sizeof(dns_hostname), "%s", hostname);
    dns_addrtype = address_type;
    if (cancel_inside_dns) {
        assert(panel_update_i10_dns_cancel(cancel_handle) ==
               PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    }
    if (dns_result == ERR_OK) {
        *address = immediate_address;
    } else if (dns_result == ERR_INPROGRESS) {
        assert(dns_count < sizeof(dns_items) / sizeof(dns_items[0]));
        dns_items[dns_count++] = (queued_dns_t){found, argument};
    }
    return dns_result;
}

static void run_scheduled(size_t index)
{
    assert(index < queue_count && queue_items[index].fn != NULL);
    queue_items[index].fn(queue_items[index].arg);
}

static ip_addr_t ipv4(unsigned a, unsigned b, unsigned c, unsigned d)
{
    ip_addr_t ip = {.is_v4 = true, .v4 = {{(unsigned char)a,
        (unsigned char)b, (unsigned char)c, (unsigned char)d}}};
    return ip;
}

static panel_update_i10_handle_t start_request(void)
{
    panel_update_i10_handle_t handle;
    assert(panel_update_i10_dns_start(&handle) ==
           PANEL_UPDATE_I10_START_ACCEPTED);
    return handle;
}

static void assert_https_not_ready(panel_update_i10_handle_t handle)
{
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(handle, ip) !=
           PANEL_UPDATE_I10_RESULT_IPV4);
    assert(ip[0] == '\0');
}

static void test_schedule_failures(void)
{
    panel_update_i10_handle_t handle;
    schedule_result = ERR_MEM;
    assert(panel_update_i10_dns_start(&handle) ==
           PANEL_UPDATE_I10_START_SCHEDULING_FAILED);
    assert(queue_count == 0U && dns_call_count == 0U);
    schedule_result = -42;
    assert(panel_update_i10_dns_start(&handle) ==
           PANEL_UPDATE_I10_START_SCHEDULING_FAILED);
    assert(queue_count == 0U && dns_call_count == 0U);
}

static void test_immediate_dns_hit_and_copy(void)
{
    dns_result = ERR_OK;
    immediate_address = ipv4(185, 199, 108, 153);
    panel_update_i10_handle_t h = start_request();
    assert(dns_call_count == 0U); /* caller only queues into tcpip thread */
    run_scheduled(0U);
    assert(dns_call_count == 1U);
    assert(strcmp(dns_hostname, PANEL_UPDATE_I10_DNS_HOST) == 0);
    assert(dns_addrtype == LWIP_DNS_ADDRTYPE_IPV4);
    /* Mutating the resolver's temporary result after return cannot alter copy. */
    immediate_address = ipv4(1, 2, 3, 4);
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_IPV4);
    assert(strcmp(ip, "185.199.108.153") == 0);
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_STALE);
}

static void test_async_success_duplicate_and_failure(void)
{
    dns_result = ERR_INPROGRESS;
    panel_update_i10_handle_t h = start_request();
    run_scheduled(0U);
    assert(dns_count == 1U);
    ip_addr_t answer = ipv4(185, 199, 108, 154);
    dns_items[0].fn(PANEL_UPDATE_I10_DNS_HOST, &answer, dns_items[0].arg);
    memset(&answer, 0, sizeof(answer));
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_IPV4);
    assert(strcmp(ip, "185.199.108.154") == 0);
    dns_items[0].fn(PANEL_UPDATE_I10_DNS_HOST, &answer, dns_items[0].arg);

    h = start_request();
    run_scheduled(1U);
    dns_items[1].fn(PANEL_UPDATE_I10_DNS_HOST, NULL, dns_items[1].arg);
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_DNS_FAILED);

    h = start_request();
    dns_result = ERR_ARG;
    run_scheduled(2U);
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_DNS_FAILED);
}

static void test_cancel_before_dispatch_and_during_lookup(void)
{
    panel_update_i10_handle_t h = start_request();
    assert(panel_update_i10_dns_cancel(h) == PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    run_scheduled(0U);
    assert_https_not_ready(h);
    assert(dns_call_count == 0U);

    h = start_request();
    cancel_handle = h;
    cancel_inside_dns = true;
    dns_result = ERR_INPROGRESS;
    run_scheduled(1U);
    cancel_inside_dns = false;
    assert(panel_update_i10_dns_cancel(h) == PANEL_UPDATE_I10_CANCEL_ALREADY_STALE);
    ip_addr_t answer = ipv4(185, 199, 108, 155);
    dns_items[0].fn(PANEL_UPDATE_I10_DNS_HOST, &answer, dns_items[0].arg);
    assert_https_not_ready(h);
}

static void test_wifi_loss_late_result_and_slot_reuse(void)
{
    dns_result = ERR_INPROGRESS;
    panel_update_i10_handle_t old = start_request();
    run_scheduled(0U);
    assert(panel_update_i10_dns_cancel(old) == PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    panel_update_i10_handle_t current = start_request();
    assert(current.slot == old.slot && current.generation != old.generation);
    run_scheduled(1U);
    ip_addr_t late = ipv4(185, 199, 108, 156);
    dns_items[0].fn(PANEL_UPDATE_I10_DNS_HOST, &late, dns_items[0].arg);
    assert_https_not_ready(old);
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(current, ip) == PANEL_UPDATE_I10_RESULT_PENDING);
    dns_items[1].fn(PANEL_UPDATE_I10_DNS_HOST, &late, dns_items[1].arg);
    assert(panel_update_i10_dns_take_result(current, ip) == PANEL_UPDATE_I10_RESULT_IPV4);
    assert(strcmp(ip, "185.199.108.156") == 0);
}

static void test_abandoned_result_and_bounded_capacity(void)
{
    dns_result = ERR_OK;
    immediate_address = ipv4(185, 199, 108, 157);
    panel_update_i10_handle_t h = start_request();
    run_scheduled(0U);
    assert(panel_update_i10_dns_cancel(h) == PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    assert_https_not_ready(h); /* OTA worker missed/purposely abandoned result */

    dns_result = ERR_INPROGRESS;
    panel_update_i10_handle_t handles[PANEL_UPDATE_I10_REQUEST_SLOT_COUNT];
    for (size_t i = 0U; i < PANEL_UPDATE_I10_REQUEST_SLOT_COUNT; ++i) {
        handles[i] = start_request();
        run_scheduled(1U + i);
    }
    panel_update_i10_handle_t extra;
    assert(panel_update_i10_dns_start(&extra) ==
           PANEL_UPDATE_I10_START_NO_REQUEST_SLOT);
    for (size_t i = 0U; i < PANEL_UPDATE_I10_REQUEST_SLOT_COUNT; ++i) {
        assert(panel_update_i10_dns_cancel(handles[i]) ==
               PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    }
    for (size_t i = 0U; i < PANEL_UPDATE_I10_DNS_TICKET_COUNT; ++i) {
        panel_update_i10_handle_t x = start_request();
        run_scheduled(3U + i);
        assert(panel_update_i10_dns_cancel(x) == PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE);
    }
    panel_update_i10_handle_t x = start_request();
    run_scheduled(19U);
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(x, ip) == PANEL_UPDATE_I10_RESULT_DNS_FAILED);
    assert(dns_count == PANEL_UPDATE_I10_DNS_TICKET_COUNT);
}

static void test_ipv6_rejected_and_scheduler_capacity(void)
{
    dns_result = ERR_OK;
    immediate_address = (ip_addr_t){.is_v4 = false};
    panel_update_i10_handle_t h = start_request();
    run_scheduled(0U);
    char ip[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY];
    assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_DNS_FAILED);

    dns_result = ERR_ARG;
    for (size_t i = 0U; i < PANEL_UPDATE_I10_SCHEDULER_TICKET_COUNT - 1U; ++i) {
        h = start_request();
        run_scheduled(i + 1U);
        assert(panel_update_i10_dns_take_result(h, ip) == PANEL_UPDATE_I10_RESULT_DNS_FAILED);
    }
    panel_update_i10_handle_t exhausted;
    assert(panel_update_i10_dns_start(&exhausted) ==
           PANEL_UPDATE_I10_START_NO_SCHEDULER_TICKET);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "schedule") == 0) test_schedule_failures();
    else if (strcmp(argv[1], "immediate") == 0) test_immediate_dns_hit_and_copy();
    else if (strcmp(argv[1], "async") == 0) test_async_success_duplicate_and_failure();
    else if (strcmp(argv[1], "cancel") == 0) test_cancel_before_dispatch_and_during_lookup();
    else if (strcmp(argv[1], "reuse") == 0) test_wifi_loss_late_result_and_slot_reuse();
    else if (strcmp(argv[1], "capacity") == 0) test_abandoned_result_and_bounded_capacity();
    else if (strcmp(argv[1], "scheduler-capacity") == 0) test_ipv6_rejected_and_scheduler_capacity();
    else return 2;
    puts("PASS: Patch116I10 DNS bridge scenario");
    return 0;
}
