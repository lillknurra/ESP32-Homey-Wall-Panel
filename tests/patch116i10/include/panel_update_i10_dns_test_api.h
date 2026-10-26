#pragma once

/* Minimal deterministic lwIP/tcpip surface for Patch116I10 host tests. */
#include <stdbool.h>

typedef int err_t;
typedef struct { unsigned char bytes[4]; } ip4_addr_t;
typedef struct { bool is_v4; ip4_addr_t v4; } ip_addr_t;
typedef void (*tcpip_callback_fn)(void *);
typedef void (*dns_found_callback)(const char *, const ip_addr_t *, void *);

#define ERR_OK 0
#define ERR_MEM -1
#define ERR_INPROGRESS -5
#define ERR_ARG -16
#define LWIP_DNS_ADDRTYPE_IPV4 0
#define IP_IS_V4(a) ((a) != NULL && (a)->is_v4)
#define ip_2_ip4(a) (&((a)->v4))
#define ip4_addr1(a) ((a)->bytes[0])
#define ip4_addr2(a) ((a)->bytes[1])
#define ip4_addr3(a) ((a)->bytes[2])
#define ip4_addr4(a) ((a)->bytes[3])

err_t tcpip_try_callback(tcpip_callback_fn function, void *argument);
err_t dns_gethostbyname_addrtype(const char *hostname, ip_addr_t *address,
                                 dns_found_callback found, void *argument,
                                 unsigned char address_type);
