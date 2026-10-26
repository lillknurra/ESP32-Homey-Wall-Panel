# Patch116I9 — Resolved HTTPS manifest endpoint policy (offline only)

## Scope and status

This patch adds a pure-C policy helper and host tests. It accepts only canonical
public-unicast IPv4 literals and the existing `stable`/`beta` manifest paths
for `lillknurra.github.io`. It builds an HTTPS URL using the literal address
while retaining the trusted DNS hostname for TLS and HTTP identity. It makes no
DNS, TLS, HTTP, Homey, OTA, or flash call. The source is not registered in the
ESP-IDF component CMake list; I9 has no native firmware linkage or activation.

The endpoint result requires a future adapter to set
`disable_auto_redirect=true`, leave
`skip_cert_common_name_check=false`, attach the configured certificate bundle,
and set HTTP `is_async=true`. These policy fields do not implement or enforce
those runtime settings on their own. With `esp_http_client`'s URL set to a
numeric IP, the client initially constructs a Host header from that IP; a
future adapter must explicitly replace it with `lillknurra.github.io` using
`esp_http_client_set_header()` before any request. The v6.0.1 header store
replaces an existing value for the same header key.

## ESP-IDF v6.0.1 and lwIP source audit

- The local IDF tree reports `v6.0.1`. Its pinned lwIP source is commit
  `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e8`, version `2.2.0-esp` development
  (`LWIP_VERSION_STRING` is `2.2.0d`).
- `dns_gethostbyname_addrtype()` is an asynchronous raw lwIP API. If the name
  is cached or parses as an address it returns `ERR_OK` synchronously and does
  not invoke the callback. When it returns `ERR_INPROGRESS`, the callback may
  later report success, failure or timeout. This project has
  `CONFIG_LWIP_TCPIP_CORE_LOCKING` disabled, so raw DNS calls require TCP/IP
  thread context. No cancellation API is provided for an outstanding query.
  Its callback cookie must therefore outlive the callback, or be held in a
  generation-fenced immutable tombstone until callback completion. I9 does not
  implement that resolver or its lifetime management.
- `esp_tls_hostname_to_fd()` in this IDF calls `getaddrinfo()` synchronously
  before creating the socket, even when the HTTP client uses async mode. Its
  hints do not set `AI_NUMERICHOST`. With an IPv4 literal, lwIP's
  `dns_gethostbyname_addrtype()` recognizes `ipaddr_aton()` before DNS lookup,
  so this is a synchronous numeric conversion/core handoff, not a second DNS
  packet. It can still wait for TCP/IP core processing; I9 establishes no
  wall-clock bound for it.
- `esp_http_client_config_t.common_name` is passed to esp-tls. The pinned
  mbedTLS path calls `mbedtls_ssl_set_hostname()` with that value when common
  name checks are enabled. This supplies the hostname used for certificate
  verification and TLS SNI. `skip_cert_common_name_check` must remain false;
  the bundle attach path enables `MBEDTLS_SSL_VERIFY_REQUIRED` when the bundle
  is configured and enabled.
- `esp_http_client_set_header()` delegates to the header store, which replaces
  an existing Host value. Therefore an adapter can preserve the numeric
  transport URL and explicitly send the original allowlisted HTTP Host.
- `disable_auto_redirect=true` prevents the client from following 3xx
  responses. A future adapter must treat the redirect response as failure.
  `is_async=true` makes supported HTTP/TLS progress calls nonblocking on
  EAGAIN/in-progress; it does not remove the synchronous `getaddrinfo()` path
  and is not an absolute operation deadline.

Potentially blocking work remains in the initial `getaddrinfo()`/TCPIP handoff,
socket creation/connect progress, TLS handshake progress, and HTTP reads. The
actual lwIP asynchronous DNS callback also has no cancellation primitive. No
25 ms call bound or 100 ms cleanup bound from I8 is measured or asserted here.

## Input and output policy

The policy rejects noncanonical IPv4 syntax, IPv6, hostnames, zones, ports,
path/query/fragment/userinfo and CR/LF injection. It rejects private, shared,
loopback, link-local, special-use, documentation, benchmarking, multicast,
limited-broadcast and reserved IPv4 ranges. It accepts only exact channel
strings `stable` or `beta` and emits the corresponding pre-existing manifest
path. The trusted TLS name and Host header are fixed constants, never derived
from the numeric address or caller text.

These checks constrain the endpoint material but do not authenticate the DNS
answer by themselves. TLS certificate verification and the exact Host/SNI
settings remain mandatory in any future network adapter. DNS answer changes,
IPv6, actual transport execution, callback lifetime integration, bounded
Homey handoff latency and runtime resource effects remain unverified.

## Validation

Run `python3 scripts/run_patch116i9_endpoint_tests.py` for host-only positive
and negative tests. This checks string/channel/size validation, public versus
special IPv4 ranges, canonical URL/path generation, fixed TLS/Host identity,
required TLS/redirect/async policy values and absence from firmware CMake. It
does not perform network or device operations and does not validate runtime
TLS/DNS behavior.
