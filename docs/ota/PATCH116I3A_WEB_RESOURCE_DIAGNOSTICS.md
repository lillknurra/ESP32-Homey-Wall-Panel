# Patch116I3A — passive web resource diagnostics

This patch prepares a **default-off** diagnostic extension. No flash, USB serial
connection, Homey command or update server request is authorized by it.

**User-visible change only in guarded audit builds:** A successful GET to the
**existing** `/homey/live-status?diagnostics=1` route gains a `resource_probe`
object. The normal `/homey/live-status` payload is unchanged. No new endpoint.

- No sample yet: `"resource_probe":{"valid":false}`.
- Sample available: `valid`, `stale`, `monotonic_ms`, `age_ms`,
  `internal_free_bytes`, `internal_largest_block_bytes`,
  `historical_min_internal_free_bytes`, `rotation_stack_margin_bytes`,
  `wifi_online`, `ui_allocated`, `homey_ready`, and `reset_reason` (integer
  returned by ESP-IDF's `esp_reset_reason()`). `stale=true` after 180 s.
- The `homey_ready` field describes the state at the time of the last passive
  sample. It does NOT invoke or control Homey.
- JSON is bounded by an additional 512-byte stack fragment. It is skipped
  safely if the existing response buffer has insufficient space; the
  diagnostic-only endpoint retains its prior payload. Samples are cached in a
  bounded static object and copied under a short spinlock; JSON formatting is
  done after release of the lock.
- `reset_reason` is a diagnostic clue, not proof that USB/RTS/DTR caused Homey
  retrying. A power-cycle versus an esptool reset remains a different
  boot/reset path and the Homey transient could have been unrelated.
- Security boundary: the pre-existing diagnostic HTTP route may be reachable
  to LAN peers. This extension contains numeric resource values and booleans,
  never tokens, IDs, URLs, IP addresses or Homey names. Use existing LAN
  protections; do not expose the panel to the public Internet.

## Build gates

All features remain disabled by default. Both the main Homey HTTP translation
unit and the observer source must have `PANEL_UPDATE_I3_OBSERVER_ENABLED=1`,
`PANEL_UPDATE_I3_AUDIT_ONLY=1`; the HTTP translation unit additionally requires
`PANEL_UPDATE_I3A_WEB_DIAG_ENABLED=1` and `PANEL_UPDATE_I3A_AUDIT_ONLY=1`.

`PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED=0`,
`PANEL_UPDATE_READONLY_WORKER_ENABLED=0`,
`PANEL_UPDATE_MANIFEST_HTTPS_ENABLED=0`; all OTA writing remains disabled.
The source already has `.operator_trial_approved = false` at the rotation call
site. None of these flags authorize a physical action.

## Compatibility with physical Patch116I3 measurement build

Codex added a monotonic timestamp to the *log line* when making that image.
Patch116I3A changes the branch just before the log statement but intentionally
leaves that log line unchanged. Native Mac validation must confirm the exact
uncommitted source diff in the isolated worktree before applying.

## Validation sequence

1. Verify Git base, overlay and original checkout preservation.
2. Check file hashes, `git apply --check --whitespace=error`.
3. Run `python3 scripts/run_patch116i3a_web_diag_tests.py` and existing I3/I2/I1/I tests.
4. Compile guarded active branch against real ESP-IDF 6.0.1 headers, with
   auditor-approved flags only; verify the disabled branch links without
   these symbols and has no runtime behavior change.
5. Clean **default-off** ESP32-S3 build; optionally build **audit-only**
   observer+web candidate for an *offline* image validation. No flash.
6. Confirm `partitions.csv`, `sdkconfig`, Homey, markis and RGB/LVGL unchanged.

## Future physical step (not authorized)

Only after reviewing the exact audit binary and a new flash authorization,
replace the current Patch116I3 app with a diagnostic build. Observe the panel
in ordinary operation, read only the existing diagnostic GET using a trusted
LAN client, and compare heap/stack values over time. Reset must not be
implicitly requested. No new HTTPS outbound update check until those results
are reviewed.
