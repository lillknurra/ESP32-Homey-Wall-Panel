# Patch116I10 – bounded-lifetime asynchronous DNS bridge (offline only)

## Scope and status

This is a standalone C adapter and deterministic host model. The adapter is
not listed in `components/panel_update_service/CMakeLists.txt`, is not called
by the update worker or Homey code, and performs no DNS traffic during host
tests. OTA and all update network activity remain disabled. Patch113 remains
the latest verified production firmware. Nothing here establishes DNS timing,
runtime behavior, or an end-to-end HTTPS deadline.

## ESP-IDF v6.0.1 / lwIP source audit

The locked tree is lwIP commit
`fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e8` (version string
`2.2.0d`). `tcpip_try_callback()` is declared in
`components/lwip/lwip/src/include/lwip/tcpip.h` and implemented in
`components/lwip/lwip/src/api/tcpip.c`. It posts a callback to the TCP/IP
mailbox without waiting; mailbox allocation/post failure returns `ERR_MEM`.
The adapter handles every non-`ERR_OK` return immediately and does not retry.

`dns_gethostbyname_addrtype()` is declared in `lwip/dns.h`, implemented in
`lwip/src/core/dns.c`, and its callback has signature
`(const char *, const ip_addr_t *, void *)`. The adapter invokes it only from
the function scheduled through `tcpip_try_callback()`, i.e. TCP/IP-thread
context. It requests `LWIP_DNS_ADDRTYPE_IPV4`. `ERR_OK` is handled as an
immediate address result, `ERR_INPROGRESS` leaves a callback ticket alive,
and all other return values become a failed result. The callback's address
pointer refers to lwIP resolver state; the adapter copies the four address
octets to its own bounded text buffer before returning. A NULL callback
address is treated as DNS failure/timeout.

The audited effective ESP-IDF build configuration has
`CONFIG_LWIP_HOOK_DNS_EXT_RESOLVE_NONE=y`; the optional external DNS resolver
hook is not selected for this evidence. That avoids a custom hook with a
different synchronous-callback contract.

The raw lwIP resolver has no per-request cancellation operation. Therefore
cancellation invalidates the request slot immediately but does not reclaim a
callback ticket that might still be referenced. Scheduler tickets (32) and
DNS callback tickets (16) are fixed static storage. A ticket that may have
escaped to lwIP is never reused during that boot. If callbacks never arrive,
these fixed pools eventually exhaust and new DNS requests fail closed; memory
does not grow and no retry loop is started. The two request slots can still be
released immediately, so a pending DNS-only operation does not retain the I7
Homey/network owner lock. A stale DNS result is fenced by its immutable slot
index and 64-bit generation and cannot authorize HTTPS.

The scheduled function rechecks cancellation immediately before entering the
raw resolver, so cancellation before dispatch (or before that check) prevents
DNS work. Cancellation racing after this final check cannot cancel lwIP's
request; it only guarantees that the callback is discarded and cannot produce
an HTTPS-eligible result. I10 itself owns no Homey/network lock.

No user callback, TLS/HTTP operation, blocking wait, queue post, or network
owner operation runs from the DNS callback. The OTA worker polls the result
through `panel_update_i10_dns_take_result()` and must independently validate
that its own request/session remains current before using an IPv4 result.
Canceling or abandoning a request makes later take attempts stale. I10 does
not itself start HTTPS.

## Limits and unresolved runtime concerns

`tcpip_try_callback()` avoids waiting for mailbox capacity but does not make
the TCP/IP thread execute the callback by an absolute deadline. The DNS
resolver may wait for network responses until lwIP's configured DNS timeout;
I10 does not cancel that resolver entry. Ticket exhaustion is the bounded
fail-closed response. This adapter does not prove a 25 ms or 100 ms bound and
must not be represented as satisfying I8's host design limits.

ESP-IDF v6.0.1 ESP-TLS still uses synchronous `getaddrinfo()` in its
connection path. I9's numeric IPv4 endpoint avoids sending a second DNS
query, but no absolute bound on TCP/IP-core processing or later TLS calls is
established. A future integration must retain the trusted TLS/HTTP hostname,
perform no transport until the worker revalidates generation and authorization,
and solve ownership, Wi-Fi loss, TLS cancellation, and end-to-end deadline
requirements separately.

## Evidence boundary

The host tests use fake `tcpip_try_callback()` and DNS functions. Sanitizer
results prove only the host model's memory behavior for tested schedules.
They do not prove lwIP runtime scheduling, callback timing, DNS server
behavior, HTTPS, Homey priority under live network load, or hardware safety.
No native firmware linkage, firmware build, runtime DNS request, or device
operation is part of I10.
