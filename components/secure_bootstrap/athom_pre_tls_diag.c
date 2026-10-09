#include "athom_pre_tls_diag.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef PATCH065_HOST_TEST
#include "test_patch065_platform.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_tls.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mbedtls/ssl.h"
#endif

/* Linker wrappers forward each existing call exactly once. The observing task
 * owns this workspace until finish; no GET reads it. Other tasks only read the
 * owner under the mux and forward without touching the workspace. */
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_owner;
static athom_pre_tls_diagnostic_t s_diag;
static bool s_connection_window;
static int s_fd = -1; /* Private, never serialized. */
static int64_t s_connect_start;

static bool owned(void)
{
    portENTER_CRITICAL(&s_mux);
    bool result = s_owner != NULL && s_owner == xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_mux);
    return result;
}

static int bounded_error(int value)
{
    return value >= -4095 && value <= 4095 ? value : -1;
}

bool athom_pre_tls_diag_begin(bool cloud, uint32_t request_sequence)
{
    if (!cloud) return false;
    int saved_errno = errno;
    portENTER_CRITICAL(&s_mux);
    bool acquired = s_owner == NULL;
    if (acquired) s_owner = xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_mux);
    if (acquired) {
        memset(&s_diag, 0, sizeof(s_diag));
        s_diag.valid = true;
        s_diag.request_sequence = request_sequence;
        s_connection_window = false;
        s_fd = -1;
        esp_netif_t *netif = esp_netif_get_default_netif();
        s_diag.readiness_observed = true;
        s_diag.default_route_present = netif != NULL;
        if (netif != NULL) {
            esp_netif_ip_info_t ip = {0};
            esp_netif_dns_info_t dns = {0};
            s_diag.netif_up = esp_netif_is_netif_up(netif);
            s_diag.ip_assigned = esp_netif_get_ip_info(netif, &ip) == ESP_OK && ip.ip.addr != 0;
            for (int i = ESP_NETIF_DNS_MAIN; i <= ESP_NETIF_DNS_FALLBACK; ++i) {
                if (esp_netif_get_dns_info(netif, (esp_netif_dns_type_t)i, &dns) == ESP_OK &&
                    !ESP_IP_IS_ANY(dns.ip)) s_diag.dns_server_configured = true;
            }
        }
        wifi_ap_record_t ap = {0};
        s_diag.wifi_associated = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
        memset(&ap, 0, sizeof(ap));
    }
    errno = saved_errno;
    return acquired;
}

void athom_pre_tls_diag_finish(bool acquired, athom_pre_tls_diagnostic_t *out)
{
    int saved_errno = errno;
    if (out != NULL) memset(out, 0, sizeof(*out));
    if (acquired && owned()) {
        if (out != NULL) *out = s_diag;
        memset(&s_diag, 0, sizeof(s_diag));
        s_fd = -1;
        s_connection_window = false;
        portENTER_CRITICAL(&s_mux);
        s_owner = NULL;
        portEXIT_CRITICAL(&s_mux);
    }
    errno = saved_errno;
}

int __real_esp_tls_conn_new_sync(const char *, int, int, const esp_tls_cfg_t *, esp_tls_t *);
int __wrap_esp_tls_conn_new_sync(const char *host, int length, int port,
                               const esp_tls_cfg_t *cfg, esp_tls_t *tls)
{
    int entry_errno = errno;
    bool capture = owned();
    if (capture) {
        const uint32_t count = s_diag.connection_count + 1U;
        /* If redirects reconnect, report the latest connection as one coherent
         * sub-attempt rather than mixing stages from different connections. */
        memset((char *)&s_diag + offsetof(athom_pre_tls_diagnostic_t, connection_called),
               0, sizeof(s_diag) - offsetof(athom_pre_tls_diagnostic_t, connection_called));
        s_diag.connection_count = count;
        s_diag.connection_called = true;
        s_fd = -1;
        s_connection_window = true;
    }
    errno = entry_errno;
    int result = __real_esp_tls_conn_new_sync(host, length, port, cfg, tls);
    int saved_errno = errno;
    if (capture) {
        s_diag.connection_result = result;
        s_diag.connection_completed = result == 1;
        s_connection_window = false;
    }
    errno = saved_errno;
    return result;
}

int __real_lwip_getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **);
int __wrap_lwip_getaddrinfo(const char *host, const char *service,
                          const struct addrinfo *hints, struct addrinfo **res)
{
    int result = __real_lwip_getaddrinfo(host, service, hints, res);
    int saved_errno = errno;
    if (owned() && s_connection_window) {
        s_diag.dns_started = true;
        s_diag.dns_code = bounded_error(result);
        s_diag.dns_ok = result == 0 && res != NULL && *res != NULL;
        if (s_diag.dns_ok) {
            for (struct addrinfo *p = *res; p != NULL && s_diag.dns_result_count < 65535U; p = p->ai_next)
                ++s_diag.dns_result_count;
            s_diag.address_family = (*res)->ai_family == AF_INET ? 4U :
                                   ((*res)->ai_family == AF_INET6 ? 6U : 0U);
        }
    }
    errno = saved_errno;
    return result;
}

int __real_lwip_socket(int, int, int);
int __wrap_lwip_socket(int family, int type, int protocol)
{
    int result = __real_lwip_socket(family, type, protocol);
    int saved_errno = errno;
    if (owned() && s_connection_window && s_diag.dns_ok) {
        s_diag.socket_attempted = true;
        s_diag.socket_created = result >= 0;
        s_diag.socket_error = result < 0 ? bounded_error(saved_errno) : 0;
        s_fd = result;
    }
    errno = saved_errno;
    return result;
}

int __real_lwip_connect(int, const struct sockaddr *, socklen_t);
int __wrap_lwip_connect(int fd, const struct sockaddr *address, socklen_t length)
{
    int entry_errno = errno;
    bool capture = owned() && s_connection_window && s_diag.socket_created && fd == s_fd;
    if (capture) s_connect_start = esp_timer_get_time();
    errno = entry_errno;
    int result = __real_lwip_connect(fd, address, length);
    int saved_errno = errno;
    if (capture) {
        s_diag.connect_started = true;
        s_diag.connect_error = result < 0 ? bounded_error(saved_errno) : 0;
        s_diag.connect_pending = result < 0 && saved_errno == EINPROGRESS;
        s_diag.tcp_connected = result == 0;
        s_diag.connect_elapsed_ms = (uint32_t)((esp_timer_get_time() - s_connect_start) / 1000);
    }
    errno = saved_errno;
    return result;
}

int __real_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int __wrap_select(int n, fd_set *reads, fd_set *writes, fd_set *errors, struct timeval *timeout)
{
    int entry_errno = errno;
    bool capture = owned() && s_connection_window && s_diag.connect_pending &&
        !s_diag.tcp_connected && s_fd >= 0 && s_fd < FD_SETSIZE &&
        writes != NULL && FD_ISSET(s_fd, writes);
    errno = entry_errno;
    int result = __real_select(n, reads, writes, errors, timeout);
    int saved_errno = errno;
    if (capture) {
        s_diag.wait_observed = true;
        s_diag.wait_timeout = result == 0;
        s_diag.wait_error = result < 0 ? bounded_error(saved_errno) : 0;
        s_diag.connect_elapsed_ms = (uint32_t)((esp_timer_get_time() - s_connect_start) / 1000);
    }
    errno = saved_errno;
    return result;
}

int __real_lwip_getsockopt(int, int, int, void *, socklen_t *);
int __wrap_lwip_getsockopt(int fd, int level, int option, void *value, socklen_t *length)
{
    int result = __real_lwip_getsockopt(fd, level, option, value, length);
    int saved_errno = errno;
    if (owned() && s_connection_window && s_diag.connect_started &&
        !s_diag.tcp_connected && fd == s_fd && level == SOL_SOCKET && option == SO_ERROR) {
        s_diag.so_query_error = result < 0 ? bounded_error(saved_errno) : 0;
        if (result == 0 && value != NULL && length != NULL && *length >= sizeof(int)) {
            s_diag.so_error_observed = true;
            s_diag.so_error = bounded_error(*(int *)value);
            s_diag.tcp_connected = *(int *)value == 0;
        }
        s_diag.connect_elapsed_ms = (uint32_t)((esp_timer_get_time() - s_connect_start) / 1000);
    }
    errno = saved_errno;
    return result;
}

int __real_mbedtls_ssl_setup(mbedtls_ssl_context *, const mbedtls_ssl_config *);
int __wrap_mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *cfg)
{
    int saved_errno = errno;
    if (owned() && s_connection_window) s_diag.tls_setup_started = true;
    errno = saved_errno;
    return __real_mbedtls_ssl_setup(ssl, cfg);
}

static const char *result_class(const athom_pre_tls_diagnostic_t *d)
{
    if (!d->valid) return "not_observed";
    if (!d->connection_called) return "no_new_connection";
    if (d->connection_completed) return "tls_connected";
    if (d->tls_setup_started) return "tls_setup_reached";
    if (d->tcp_connected) return "tcp_connected";
    if (d->so_error_observed && d->so_error != 0) return "connect_error";
    if (d->so_query_error != 0) return "so_error_query_failed";
    if (d->wait_error != 0) return "connect_wait_error";
    if (d->wait_timeout) return "connect_wait_timeout";
    if (d->connect_started) return d->connect_pending ? "connect_pending" : "connect_error";
    if (d->socket_attempted && !d->socket_created) return "socket_error";
    if (d->dns_started && !d->dns_ok) return "dns_error";
    return "unknown";
}

bool athom_pre_tls_diag_json(const athom_pre_tls_diagnostic_t *d, char *out, size_t capacity)
{
    if (d == NULL || out == NULL || capacity == 0) return false;
    int n;
#define B(field) (d->field ? "true" : "false")
    if (!d->valid) n = snprintf(out, capacity, "{\"valid\":false}");
    else n = snprintf(out, capacity,
        "{\"valid\":true,\"result\":\"%s\",\"request_sequence\":%u,\"readiness_observed\":%s,"
        "\"default_route_present\":%s,\"netif_up\":%s,\"ip_assigned\":%s,"
        "\"dns_server_configured\":%s,\"wifi_associated\":%s,"
        "\"connection_called\":%s,\"connection_count\":%u,\"connection_completed\":%s,\"connection_result\":%d,"
        "\"dns_started\":%s,\"dns_ok\":%s,\"dns_code\":%d,\"dns_result_count\":%u,"
        "\"address_family\":%u,\"socket_attempted\":%s,\"socket_created\":%s,\"socket_error\":%d,"
        "\"connect_started\":%s,\"connect_pending\":%s,\"connect_error\":%d,"
        "\"wait_observed\":%s,\"wait_timeout\":%s,\"wait_error\":%d,"
        "\"so_error_observed\":%s,\"so_error\":%d,\"so_query_error\":%d,"
        "\"tcp_connected\":%s,\"connect_elapsed_ms\":%u,\"tls_setup_started\":%s}",
        result_class(d), (unsigned)d->request_sequence, B(readiness_observed), B(default_route_present),
        B(netif_up), B(ip_assigned), B(dns_server_configured), B(wifi_associated),
        B(connection_called), (unsigned)d->connection_count, B(connection_completed), d->connection_result,
        B(dns_started), B(dns_ok), bounded_error(d->dns_code), (unsigned)d->dns_result_count,
        d->address_family == 4 || d->address_family == 6 ? d->address_family : 0,
        B(socket_attempted), B(socket_created), bounded_error(d->socket_error),
        B(connect_started), B(connect_pending), bounded_error(d->connect_error),
        B(wait_observed), B(wait_timeout), bounded_error(d->wait_error),
        B(so_error_observed), bounded_error(d->so_error), bounded_error(d->so_query_error),
        B(tcp_connected), (unsigned)d->connect_elapsed_ms, B(tls_setup_started));
#undef B
    if (n < 0 || (size_t)n >= capacity) { out[0] = '\0'; return false; }
    return true;
}
