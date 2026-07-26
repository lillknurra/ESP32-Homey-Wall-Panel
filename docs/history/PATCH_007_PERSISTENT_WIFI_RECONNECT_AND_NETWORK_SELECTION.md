# Patch 007 - Persistent Wi-Fi Reconnect and Network Selection

## Status

LOCAL IMPLEMENTATION COMPLETE / OFFLINE VALIDATION PASS / HARDWARE RUNTIME PASS /
FINAL REVIEW READY / NOT COMMITTED / NOT PUBLISHED / NOT MERGED

## Baseline

- Base branch: `main`
- Starting commit: `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`
- Working branch: `patch-007-persistent-wifi-reconnect`
- Stable remote baseline at final-review capture:
  `origin/main` = `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`

## Purpose

Preserve a verified Wi-Fi configuration across restart, try the saved network
before opening provisioning, provide scanned network selection and explicit
reconfiguration, and commit candidate credentials transactionally only after a
successful connection.

## Implemented behavior

- Saved STA configuration is read from flash at boot.
- A valid saved SSID is attempted before setup AP is opened.
- Network scans are asynchronous, deduplicated and strongest-first.
- Hidden or manually entered networks use `Annat nätverk...`.
- Candidate credentials remain in RAM while being tested.
- A successful candidate is persisted only after IP acquisition and persistent
  readback verification.
- A failed candidate receives exactly five bounded attempts before the previous
  saved configuration is restored.
- The candidate buffer is cleared during restore.
- The panel returns automatically to ONLINE after successful restore.
- `Ändra Wi-Fi` opens provisioning from ONLINE without erasing the saved
  configuration.
- APSTA preserves the existing STA path while setup AP is active.
- The panel code is prefilled only through setup AP and remains empty through
  STA/LAN.
- Physical BOOT hold for five seconds remains the only full Wi-Fi wipe path.
- The setup view is Swedish and uses the project-owned 22 px LVGL font for the
  required Swedish glyphs.
- The setup code includes an `MM:SS` lifetime countdown and rotates without
  introducing layout overlap.
- The submit response uses sanitized, mobile-adapted HTML with separate text for
  saved-config rollback and first-installation retry.

## Final reviewed scope

- `components/secure_bootstrap/CMakeLists.txt`
- `components/secure_bootstrap/homey_panel_font_22.c`
- `components/secure_bootstrap/include/homey_panel_font_22.h`
- `components/secure_bootstrap/include/secure_bootstrap.h`
- `components/secure_bootstrap/secure_bootstrap_esp.c`
- `components/secure_bootstrap/secure_bootstrap_logic.c`
- `components/secure_bootstrap/test_host/test_secure_bootstrap.c`
- `components/secure_bootstrap/test_host/test_wifi_state_machine.c`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/handoff/MASTER_INDEX.md`
- `docs/history/PATCH_HISTORY.md`
- `docs/history/PATCH_007_PERSISTENT_WIFI_RECONNECT_AND_NETWORK_SELECTION.md`
- `main/main.c`
- `scripts/validate_patch_007.sh`

## Validation

### Offline and build evidence

- Native secure-bootstrap host tests: PASS.
- Persistent Wi-Fi state-machine host tests: PASS.
- Patch 007 deterministic validator: PASS.
- Full ESP-IDF v6.0.1 ESP32-S3 build: PASS.
- `git diff --check`: PASS.
- Locked scope validation: PASS.
- Sensitive-pattern scan in final-review capture: no hits.

### Hardware and runtime evidence

- Saved Wi-Fi reconnect after restart: PASS.
- Cold-start saved connection before setup AP: PASS.
- Scanned network dropdown and manual-network path: PASS.
- Candidate commit-after-IP and persistent readback: PASS.
- Failed-candidate rollback to the prior saved configuration: PASS.
- Exactly five candidate attempts and no sixth attempt: OPERATOR-OBSERVED PASS.
- Automatic return to ONLINE after restore: PASS.
- Candidate-buffer clearing: OPERATOR-OBSERVED PASS.
- APSTA retains the existing STA path: PASS.
- Portal remains reachable through STA/LAN during provisioning: PASS.
- Panel-code field empty through STA/LAN: PASS.
- Panel-code field prefilled through setup AP: PASS.
- Five-second physical BOOT wipe: PASS.
- Swedish setup UI and Swedish glyph rendering: PASS.
- Mobile submit label and both sanitized status variants: PASS.
- Code countdown, code rotation, QR refresh and post-rotation layout: PASS.
- No credentials exposed in the verified UI or supplied sanitized evidence:
  PASS.

### Evidence limitation

The supplied filtered retry log contains only the final ONLINE event. It supports
the final ONLINE result but does not independently reconstruct all five attempts,
the absence of a sixth attempt or candidate-buffer clearing. Those three points
are therefore recorded as operator-observed hardware results, not as fully
artifact-reconstructed results.

## Non-goals and preserved boundaries

- Real Athom OAuth: NOT RUN.
- Live Homey discovery or inventory collection: NOT RUN.
- Homey command or mutation execution: NOT RUN.
- Direct ESP32-Homey protocol validation: NOT RUN.
- Secure Boot enablement: NOT RUN.
- Flash-encryption enablement: NOT RUN.
- eFuse writes or production-key provisioning: NOT RUN.
- Encrypted NVS and anti-rollback activation: NOT RUN.
- No credential, panel-code, SSID, BSSID, private IP or password value may be
  committed as evidence.

## Rollback

Before publication, discard the uncommitted Patch 007 working-tree changes.
After an approved commit or merge, revert the Patch 007 commit or merge normally.
Do not rewrite published history. A hardware rollback may require reflashing the
previous verified firmware and restoring Wi-Fi through the established local
provisioning path.

## Completion criteria

Local implementation and validation criteria are complete. Repository completion
still requires:

1. complete diff review;
2. exact-path staging;
3. approved local commit;
4. normal push and remote-ref verification;
5. narrow pull-request review;
6. merge and merged-`main` verification.

No commit, push, pull request or merge is claimed by this record.
