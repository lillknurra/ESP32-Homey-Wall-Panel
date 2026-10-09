#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
typedef void *TaskHandle_t;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) ((void)(p))
#define portEXIT_CRITICAL(p) ((void)(p))
#define ESP_OK 0
typedef int esp_netif_t;
typedef struct { struct {uint32_t addr;} ip; } esp_netif_ip_info_t;
typedef struct { struct {uint32_t addr;} ip; } esp_netif_dns_info_t;
typedef int esp_netif_dns_type_t;
#define ESP_NETIF_DNS_MAIN 0
#define ESP_NETIF_DNS_FALLBACK 2
#define ESP_IP_IS_ANY(ip) ((ip).addr == 0)
typedef struct {char private_bytes[64];} wifi_ap_record_t;
typedef int esp_tls_cfg_t;
typedef int esp_tls_t;
typedef struct {int private_state;} mbedtls_ssl_context;
#define MBEDTLS_PRIVATE(member) private_##member
#define MBEDTLS_ERR_SSL_WANT_READ -0x6900
#define MBEDTLS_ERR_SSL_WANT_WRITE -0x6880
enum {
MBEDTLS_SSL_HELLO_REQUEST,
MBEDTLS_SSL_CLIENT_HELLO,
MBEDTLS_SSL_SERVER_HELLO,
MBEDTLS_SSL_SERVER_CERTIFICATE,
MBEDTLS_SSL_SERVER_KEY_EXCHANGE,
MBEDTLS_SSL_CERTIFICATE_REQUEST,
MBEDTLS_SSL_SERVER_HELLO_DONE,
MBEDTLS_SSL_CLIENT_CERTIFICATE,
MBEDTLS_SSL_CLIENT_KEY_EXCHANGE,
MBEDTLS_SSL_CERTIFICATE_VERIFY,
MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC,
MBEDTLS_SSL_CLIENT_FINISHED,
MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC,
MBEDTLS_SSL_SERVER_FINISHED,
MBEDTLS_SSL_FLUSH_BUFFERS,
MBEDTLS_SSL_HANDSHAKE_WRAPUP,
MBEDTLS_SSL_NEW_SESSION_TICKET,
MBEDTLS_SSL_HANDSHAKE_OVER
};
typedef int mbedtls_ssl_config;
TaskHandle_t xTaskGetCurrentTaskHandle(void);
int64_t esp_timer_get_time(void);
esp_netif_t *esp_netif_get_default_netif(void);
bool esp_netif_is_netif_up(esp_netif_t *);
int esp_netif_get_ip_info(esp_netif_t *, esp_netif_ip_info_t *);
int esp_netif_get_dns_info(esp_netif_t *, esp_netif_dns_type_t, esp_netif_dns_info_t *);
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
