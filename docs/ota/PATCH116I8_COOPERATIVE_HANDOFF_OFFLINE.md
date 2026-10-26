# Patch116I8 - Cooperative DNS/HTTPS handoff contract (OFFLINE ONLY)

## Status and boundaries

Pure C state-machine **host model** linked against the Patch116I7 admission
policy. No native DNS resolver, TLS, HTTP, FreeRTOS, Homey operation, OTA
installation, image download, partitions, reboot or flash. The new source is
NOT part of the ESP-IDF component's CMake list. Approval to develop this
model does NOT authorize a physical/network operation or firmware activation.

## Decisions proven by the model, not on the ESP32

- The I7 shared owner is reserved before the first DNS action. Admission
  requires the separately authorized trial gate, healthy heap, Homey READY,
  a truly idle Homey phase, empty command queue, and all previously opened
  Homey/Cloud TLS connections already closed.
- An OTA session emits **one-shot actions**, not blocking function calls:
  `START_ASYNC_DNS`, `START_ASYNC_TLS`, `START_ASYNC_HTTP_GET`, followed by
  `CLOSE_TRANSPORT`. On Homey priority, missing resources, failure or expiry,
  it instead emits `ABORT_AND_CLOSE`.
- The Homey caller MUST retain/queue its command and re-attempt after OTA
  teardown. The I7 model alone cannot preserve actual panel button presses;
  existing `athom_oauth_runtime_queue_awning_step` currently returns BUSY.
- An I7 lease remains held while any DNS callback, socket, TLS context or
  HTTP client may still be alive. Merely reaching a logical deadline never
  releases ownership or a C allocation. Sessions are zero-initialized and
  reset is refused until they become terminal after verified cleanup.
- Verified cleanup must attest that all I/O has returned and the socket/HTTP/
  TLS resources are destroyed, and all DNS callback contexts are drained or
  protected by a stable tombstone. Duplicate/stale callbacks never release
  a different generation.
- If cleanup exceeds its declared acknowledgement budget, the work is
  quarantined with the lease **still held**. Later verified cleanup may
  release the lease but can never publish the timed-out result as success.
- Even a success-path CLOSE becomes cancelled if Homey priority arrives
  while teardown is running.

## Runtime feasibility blocker

**ESP-IDF 6.0.1 `esp_http_client_config_t.is_async` is insufficient by itself.**
The esp-tls connection path still calls synchronous `getaddrinfo()` during
its initial DNS resolution and can wait in `select()` even in async connect.
The proposed adapter MUST prove a bounded, cancellable split-phase DNS path
(e.g. a reviewed `dns_gethostbyname`-style asynchronous resolver) and a
bounded HTTPS/TLS progress loop before the capability flags are allowed to
be true on native hardware. DNS callback lifetime and safe cancellation are
nontrivial; NEVER pass a stack pointer to an asynchronous resolver callback,
free a still-referenced callback cookie, or reuse a cookie before an
outstanding callback is drained/tombstoned.

Provisionally the model rejects an adapter that cannot attest <=25ms per
single call and <=100ms abort-and-cleanup acknowledgement; these are HOST
DESIGN LIMITS, not measured field guarantees or a service-level promise.
A liar/misconfigured adapter can still block: this host model cannot prove
that the actual esp-idf resolver, TLS library or FreeRTOS obey those bounds.
An unresponsive adapter leaves the lease quarantined rather than risking a
use-after-free or falsely releasing a live TLS owner. Therefore this model
alone does **not** solve bounded Homey command latency.

`esp_http_client_cancel_request()` must NOT be used as a close-only abort:
ESP-IDF 6.0.1 closes then re-connects within that API. The eventual native
adapter must use a documented, validated close/cleanup path that never
opens another socket during cancellation.

## Further gates before live use

1. Resolve the difference between published `HANDOFF.md` and the uncommitted
   local Patch116I5/I7/I8 overlay before any mutation requiring Git authority.
2. Keep all existing Homey reservations under the SAME atomic arbiter/lock;
   never check the old `network_phase` and then reserve a separate OTA mutex.
   No priority inversion for OAuth, token refresh, inventory or awning write.
3. Add durable Homey command retry/queue semantics, preserving duplicate,
   freshness, generation and alias safety checks at the point of execution.
   Do not simply retry non-idempotent writes or perform stale queued actions.
4. Implement and test real nonblocking DNS cancellation/callback lifetimes,
   TLS progress, socket/HTTP cleanup (including timeout and mid-handshake).
   Verify the worst-case HOMEY demand-to-acquire time on the ESP32; the I8
   25/100ms design contracts are not runtime proof.
5. Re-check peak internal free heap, largest block, task stack, network
   task lifetimes, Wi-Fi stability, and effective single-TLS ownership.
6. Keep OTA fetch/worker default OFF, prevent new HTTP/TLS requests, retain
   signed manifest checks, and defer installation until separate authorization.
7. Operator must explicitly authorize any build, flash, runtime HTTPS,
   Homey operation, Git mutation or other external action not already covered.

## Offline validation

`python3 scripts/run_patch116i8_handoff_tests.py`

Expected PASS: simulated success closes; Homey priority at DNS/TLS/HTTP/CLOSE
stages; health/resource revocation; timeout and quarantine; stale-generation
replay rejection; cleanup not acknowledged early; native compile guard and
absence from ESP-IDF CMake. Does not exercise the device or a real DNS/TLS
stack. Run independently of Patch116I5/I7 host tests; do not rerun unrelated
passing suites without a source change.
