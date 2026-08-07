# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Stable repository state

- stable branch: `main`;
- stable merge: `b6ba6caf68ebf89c4944ea422bf768d448371f7e`;
- Patch 015 source head: `2100df609f5c364ae4a688d563489f0ecb3a0c9e`;
- Patch 015 pull request: `#22`;
- Patch 015: `COMPLETE / MERGED`;
- Patch 013 runtime: `NOT RUN`;
- Package 3B: `NOT_STARTED`;
- active implementation patch: Patch 016;
- active branch: `patch-016-live-read-only-favorite-light-status`.

Patch 015 provides private persistent Homey alias mappings bound to the exact
selected Homey identity. Host/static validation, build, size, flash and visual
runtime passed. No display or UI regression was found after restart without
serial logging. Automatic serial capture after flash must not be used because
DTR/RTS reset timing can disturb display startup. No separate Patch 015
finalization patch is to be created.

## Patch 016 active scope

Patch 016 starts from `b6ba6caf68ebf89c4944ea422bf768d448371f7e` and adds live read-only status for the two
lighting cards on `Favoriter`:

- widget `4`: `light_1/on`;
- widget `5`: `light_2/on`;
- `Tänd`, `Släckt`, `Otillgänglig`, `Okänd`, `Ej konfigurerad`;
- bounded private provisioning through the existing local phone portal and the
  Patch 015 `homey_alias_v1` store.

Raw Homey identifiers must never enter Git, UI, logs, evidence or diagnostics.
Homey mutation, clickable lighting cards, UI redesign, Package 3B, a separate
Patch 015 finalization and automatic serial capture are prohibited or out of
scope. Firmware implementation is `APPLIED_LOCALLY`; host tests, build, size and app-partition fit are `PASS`; flash, live runtime, commit, push and PR remain `NOT_RUN`.

## Next action

Define and lock the exact minimum implementation file list, then implement and
run host/static validation. Build, flash and live runtime remain separate
evidence phases.
<!-- PATCH_016_IMPLEMENTATION_STATE_BEGIN -->
## Patch 016 implementation state

- implementation: APPLIED LOCALLY
- exact implementation scope: LOCKED (18 files)
- Homey mutation: NOT IMPLEMENTED
- build: PASS
- flash: NOT RUN
- runtime: NOT RUN
- commit, push and PR: NOT RUN
<!-- PATCH_016_IMPLEMENTATION_STATE_END -->
<!-- PATCH_016_WIFI_AUTOMATIC_FALLBACK_BEGIN -->
## Patch 016 Wi-Fi automatic fallback scope extension

After `SECURE_BOOTSTRAP_WIFI_MAX_RETRIES`, an unreachable saved Wi-Fi now transitions directly to `SECURE_BOOTSTRAP_WIFI_PROVISIONING` and returns `SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING`. The existing `HomeyPanel-Setup`, panel code, QR code and `192.168.4.1` flow is reused. No Wi-Fi, Homey or alias data is erased in advance, and Homey plus `/homey/lights` remain unavailable until candidate Wi-Fi is verified and online.

The locked Patch 016 scope is exactly 18 files. The original 14-file scope was expanded by exactly four files: `components/secure_bootstrap/secure_bootstrap_logic.c`, `components/secure_bootstrap/test_host/test_secure_bootstrap.c`, `components/secure_bootstrap/include/phone_provisioning.h`, and `components/secure_bootstrap/secure_bootstrap_esp.c`. No erase-flash, physical wipe, Homey mutation or automatic serial capture is introduced.
<!-- PATCH_016_WIFI_AUTOMATIC_FALLBACK_END -->

<!-- PATCH_016_POST_FALLBACK_STATE_BEGIN -->
## Patch 016 verified local post-fallback state

- implementation: APPLIED_LOCALLY;
- locked scope: 18 files;
- host tests: PASS;
- Wi-Fi state-machine automatic fallback: PASS;
- GET and POST `/homey/lights` require Wi-Fi online plus Homey READY: PASS;
- static validator and privacy scan: PASS;
- ESP-IDF v6.0.1 build: PASS;
- size and app-partition fit: PASS;
- flash and live runtime: NOT_RUN;
- commit, push and PR: NOT_RUN.

Entering `HomeyPanel-Setup` now clears only the volatile `s_wifi_online` flag through `phone_provisioning_on_wifi_offline()`. It does not erase or mutate Wi-Fi, Homey or alias data. Lamp provisioning remains unavailable until a candidate Wi-Fi has been verified and `phone_provisioning_on_wifi_online()` has run.
<!-- PATCH_016_POST_FALLBACK_STATE_END -->

<!-- PATCH_016_DOCUMENTATION_CONSISTENCY_BEGIN -->
## Patch 016 verified local state

- implementation: `APPLIED_LOCALLY`;
- locked scope: exactly 18 files;
- original 14-file scope expanded by exactly four files:
  - `components/secure_bootstrap/secure_bootstrap_logic.c`;
  - `components/secure_bootstrap/test_host/test_secure_bootstrap.c`;
  - `components/secure_bootstrap/include/phone_provisioning.h`;
  - `components/secure_bootstrap/secure_bootstrap_esp.c`;
- host tests: `PASS`;
- Wi-Fi automatic fallback: `PASS`;
- GET and POST `/homey/lights` require Wi-Fi online and Homey READY: `PASS`;
- static validator: `PASS`;
- privacy scan: `PASS`;
- ESP-IDF v6.0.1 build: `PASS`;
- size and app-partition fit: `PASS`;
- flash and live runtime: `NOT_RUN`;
- commit, push and PR: `NOT_RUN`;
- Patch 013 runtime remains `NOT_RUN`;
- Package 3B remains `NOT_STARTED`;
- Patch 015 remains complete and merged;
- no additional state-lock or finalization patch is created for Patch 016.
<!-- PATCH_016_DOCUMENTATION_CONSISTENCY_END -->

<!-- PATCH_016_RUNTIME_ERROR_CORRECTION_BEGIN -->
## Patch 016 runtime error correction

- verified prior live runtime: `FAIL`;
- first provisioning-session code/QR overwrite: correction `APPLIED_LOCALLY`;
- stale GOT_IP filtering in provisioning: `APPLIED_LOCALLY`;
- live Homey display deferred while Wi-Fi offline: `APPLIED_LOCALLY`;
- `/homey/lights` uses the persisted live Athom selected-Homey identity with synthetic fallback: `APPLIED_LOCALLY`;
- code remains valid until candidate verification closes provisioning: `APPLIED_LOCALLY`;
- code rotation commits only after AP password update succeeds: `APPLIED_LOCALLY`;
- standard `idf.py flash` ends with an esptool hard reset via RTS; no claim of `RTS_AFTER_FLASH=NOT_CHANGED` is valid;
- flash and corrected live runtime: `NOT_RUN`;
- commit, push and PR: `NOT_RUN`.
<!-- PATCH_016_RUNTIME_ERROR_CORRECTION_END -->

<!-- PATCH_016_RUNTIME_CORRECTION_2_BEGIN -->
## Patch 016 runtime correction 2

- consumed panel codes are never reactivated; replay remains rejected;
- only one candidate session is accepted per panel code;
- candidate failure without a saved fallback opens a fresh provisioning session;
- stale `GOT_IP` events in provisioning remain ignored;
- `/homey/lights` requires three independent conditions: Wi-Fi online, authoritative Homey runtime READY, and a verified selected Homey identity;
- exact GET and POST `/homey/lights` handlers remain registered;
- focused host tests and the real `test_secure_bootstrap.c` executable are required to pass;
- full ESP-IDF v6.0.1 build, size, and app-partition fit are required;
- flash and corrected live runtime remain `NOT_RUN`;
- standard `idf.py flash` performs its final hard reset via RTS;
- commit, push, and PR remain `NOT_RUN`.
<!-- PATCH_016_RUNTIME_CORRECTION_2_END -->
