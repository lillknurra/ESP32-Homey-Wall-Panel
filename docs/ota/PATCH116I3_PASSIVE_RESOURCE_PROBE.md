# Patch116I3 - Passive Resource Probe and Single-Owner Call-Site Foundation

## Purpose

Prepare a separately authorized, read-only resource and compatibility experiment for the multi-panel central firmware availability checker. A blocked DNS/TLS operation must never cause automatic worker replacement. **This patch does not run any network request and does not approve activating Patch116I2.**

## Base and expected files

- Authoritative source reference: `origin/main` commit `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`, tree `cd482e7020c83e005ab3fbd5e78e2f67c22131e7` (verify live refs at application time).
- Prerequisite: offline Patch116, 116B, 116D, 116E, 116F, 116G, 116I, 116I1, 116I2 overlays are in the isolated Patch116 worktree. Do not apply this patch against plain main.
- Changed: `components/panel_update_service/CMakeLists.txt`, `components/secure_bootstrap/CMakeLists.txt`, `components/secure_bootstrap/secure_bootstrap_esp.c`.
- Added: `panel_update_i3_probe_policy.h/.c`, `panel_update_i3_probe.h`, `panel_update_i3_probe_esp.c`, host runner + tests, and this file.
- Do not touch `main/main.c`, `partitions.csv`, `sdkconfig`, installed firmware, or Homey/awning logic.

## Operator decision / boundaries

- All feature macros default to zero: `PANEL_UPDATE_I3_OBSERVER_ENABLED=0`, `PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED=0`.
- Any enabled observer requires `PANEL_UPDATE_I3_AUDIT_ONLY=1` at compile time. The native active path must be validated against the local ESP-IDF v6.0.1 before any physical request.
- Network-trial compilation additionally requires the existing I1/I2 HTTPS + worker audit gates. It **still cannot start from the actual call site** because `.operator_trial_approved=false` is hardcoded. A later separately reviewed, explicitly approved operator control would be necessary. Do not flip this literal based on a Git flag, HTTP request, RTC CRC, or unverified manifest.
- This architecture does not provide a hard DNS/TLS timeout, does not implement OTA flash, cannot promise a working display or Homey availability, and does not publish firmware updates. It has no UI or HTTPS endpoint.

## Passive observation design

- Exactly one call site, after LVGL unlock, in the existing `rotation_task`. Do not create an extra caller task or block the LVGL lock.
- At most one measurement every 60 seconds; no heap queries at 100-ms rotation frequency. First sample delayed one full interval; no transport object exists in observer mode.
- Measurements: internal free heap, largest internal free block, minimum internal heap since boot, the rotation task's FreeRTOS stack high-water mark, Wi-Fi readiness, UI object allocation and the already cached Homey data-readiness state.
- `ui_allocated` and `homey_ready` are passive software flags, **not proof of visible RGB output** or a successful live Homey request. Do not turn them into health acceptance for OTA rollback.
- Logging only the above coarse numbers/bools; no MAC, IP, SSID, Homey identifier, token, serial or firmware URL. No NVS/diaglog writes.
- `panel_update_i3_policy_step` is pure C, with monotonic checks, threshold checks, one-shot admission, and fail-closed counter wrap. Native adapter never calls network unless separately compiled AND independently authorized.
- Trial policy additionally requires: 3 consecutive healthy minute samples, 180 seconds since observer initialization, current Wi-Fi/UI/Homey-ready observations, internal free >=64KiB, largest internal block >=24KiB and rotation stack margin >=3072 bytes. These thresholds are *heuristic safety gates*, not evidence of sufficient TLS resource headroom.
- One-shot worker: would create a single existing I2 worker, submit `stable` once with generation 1, then poll no faster than once/second. A blocked worker is quarantined, never force deleted or replaced, and never given a second request during this trial. Its response is not shown to the operator or trusted as OTA authorization.

## Evidence / validation

- Host pure-C deterministic and adversarial policy tests: GCC and Clang, ASan/UBSan. Default-off object has zero undefined external symbols. Audit compile gates intentionally reject activation without required guards.
- Host observer and one-shot paths are compiled against API-shaped stubs and run without network activity.
- Previous Patch116I, I1, I2 regressions passed in the assembled overlay; this is **host evidence only**.
- Local scratch fixture deliberately uses an abbreviated `secure_bootstrap_esp.c` context only to verify patch syntax and hunk placement. The real ESP-IDF source, CMake/link integration and actual source `git apply --check` **must be verified by Codex in the operator's isolated worktree**.
- **No real ESP-IDF build, physical runtime, network, installation or rollback validation of I3 was executed here.**

## Next gated work

1. Verify exact repository and worktree status, authoritative handoff and all four original dirty paths.
2. Apply against the real isolated I2 overlay, confirming full context and no CMake overwrite.
3. Native compile-only with observer enabled/audit flag to establish actual `uxTaskGetStackHighWaterMark` units, stack usage, logging and IDF component linkage. Compile-only with network trial flags as a separate optional check, without running.
4. Clean full ESP-IDF default-off build, inspect map/symbols and binary size.
5. Before **any** physical trial, obtain fresh bounded operator authorization. First experiment should measure **passive observer only**. Collect bounded heap/stack and Homey/display observations; do not infer visual health from flags.
6. A subsequent *separately approved* single read-only network trial would require published, trusted release metadata, documented heap headroom, and an actual operator consent path. A stuck worker retains resources indefinitely.
7. Real OTA remains blocked by signing trust root, A/B migration, flash safety, recovery and end-to-end authorization.

## Rollback

No Git publication or device change in this package. Remove only the exact I3 additions in the isolated worktree after preserving evidence; do not touch the original checkout's dirty paths. Keep both network and observer default OFF.
