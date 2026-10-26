# Patch116I7 - Homey-priority network admission (offline-only)

## Scope

Pure-C arbiter modelling a single exclusive owner: Homey or background OTA
metadata discovery. Host tests only. No ESP-IDF source registration, task,
Wi-Fi, HTTP, TLS, Homey operation, OTA install, flash or release publication.
The model is NOT evidence that a real firmware task can interrupt blocking
DNS/TLS/HTTP or preserve Homey command latency.

## Admission prerequisites

All of the following must be attested by the future runtime adapter:
separately authorized trial, stable Wi-Fi, settled boot, Homey READY, no
Homey network phase in progress, no pending Homey command, both live Homey
and Cloud TLS transports already closed, no other outbound TLS owner, and
adequate measured internal heap, largest block and stack margin. The memory
thresholds mirror Patch116I3's passive observation limits; they are NOT a
proven TLS memory budget and cannot replace peak-runtime tests.

A single atomic shared lock MUST protect both Homey and OTA transitions. A
snapshot of existing `network_phase` does not constitute ownership. All
Homey reservations (OAuth, restore, inventory, light control, awnings,
selection, token refresh) must participate before any runtime enablement.

## Priority and cancellation

- Homey gets the owner when idle; OTA cannot reserve during any Homey work.
- An arriving Homey request while OTA owns transport sets pending and cancels
  the OTA lease. The Homey caller must retry when that lease has been released.
- OTA may not begin/continue/claim a new transport after cancellation.
- Before the first potentially blocking DNS/TLS/HTTP call the OTA worker must
  claim ownership. Ownership persists until all calls returned and the actual
  HTTP/TLS handle and socket have been fully cleaned up.
- Timeout/late-result quarantine requests cancellation but NEVER kills a task,
  frees a live context or releases the transport lock early.
- Stale or repeated generation release cannot unlock a new owner's transport.
- A withdrawn Homey request must be explicitly signalled; OTA cancellation
  is terminal, even after withdrawal.

**Known blocker:** a synchronous `esp_http_client_open()`/DNS/TLS call can
block beyond the logical timeout. The arbiter cannot forcibly interrupt it;
real Homey command latency is therefore NOT bounded by this model. Do not
activate the worker until transport I/O is proven cooperatively cancellable
with a hard latency bound, or equivalent independent isolation is validated.

## Validation

`python3 scripts/run_patch116i7_admission_tests.py`

Pass criteria: deterministic host scenarios for gates, Homey priority,
serialization, stale generations, non-preemptive cleanup and sticky cancel;
negative `ESP_PLATFORM` compile gate; production CMake does not mention the
new source. Source intentionally remains untracked/unwired until the operator
reviews the patch. Host PASS never implies real firmware/build/runtime PASS.

## Deferred integration

1. Design an adapter referencing **the same** `s_network_phase_mux`/owner
   used by all Homey operations, or a proven safe migration into an equivalent
   shared arbiter. Do not add a parallel check-then-act lock.
2. Prove Homey/Cloud transport close/reopen transitions under contention.
3. Add separate bounded/cancellable I/O or isolate OTA such that an arriving
   awning command cannot be indefinitely blocked.
4. Measure actual internal heap, contiguous block and Homey response latency
   at simultaneous worst-case transitions before any authorized field trial.
5. Require fresh, explicit operator authorization for runtime checks and every
   physical or Homey mutation. OTA installation remains outside this scope.
