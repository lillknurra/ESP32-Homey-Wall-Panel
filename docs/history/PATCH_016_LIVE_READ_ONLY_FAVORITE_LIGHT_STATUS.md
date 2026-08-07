# Patch 016 – Live Read-Only Favorite Light Status

Status: ACTIVE / IMPLEMENTATION_APPLIED_LOCALLY / LOCAL_VALIDATION_PASS / BUILD_PASS / FLASH_NOT_RUN.

## Base and branch

- stable base branch: `main`;
- exact base commit: `b6ba6caf68ebf89c4944ea422bf768d448371f7e`;
- development branch: `patch-016-live-read-only-favorite-light-status`.

## Purpose

Show authoritative live read-only state for the two lighting cards on the
`Favoriter` page without adding any Homey mutation or clickable light control.

## Fixed bindings

- widget index `4`: sanitized device alias `light_1`, capability alias `on`;
- widget index `5`: sanitized device alias `light_2`, capability alias `on`.

Required user-facing states:

- boolean true: `Tänd`;
- boolean false: `Släckt`;
- unavailable or stale: `Otillgänglig`;
- unknown, invalid or unmatched: `Okänd`;
- missing private mapping: `Ej konfigurerad`.

A missing boolean must never be interpreted as `false`.

## Private provisioning model

Patch 016 uses a bounded private provisioning path through the existing local
phone portal. The path may provision only the two lighting bindings above into
the existing Patch 015 `homey_alias_v1` store. Raw Homey device IDs and
capability IDs are transient private inputs and must never enter Git, the panel
UI, logs, generated evidence or diagnostics.

The provisioning path must validate the selected Homey identity, write through
the existing dual-slot store, verify the stored record before activation and
return sanitized success or failure only.

## Exact implementation scope

Expected implementation files are limited to the smallest necessary subset of:

- `components/secure_bootstrap/CMakeLists.txt`;
- `components/secure_bootstrap/panel_ui_model.c`;
- `components/secure_bootstrap/secure_bootstrap_esp.c`;
- `components/secure_bootstrap/include/panel_homey_light_provisioning.h`;
- `components/secure_bootstrap/panel_homey_light_provisioning.c`;
- focused host tests and runners under `components/secure_bootstrap/test_host/`;
- `scripts/validate_patch_016.sh`;
- the Patch 016 architecture, handoff and history documents.

The exact implementation file list must be locked before code application.

## Non-goals

- Homey mutation or command dispatch;
- clickable lighting cards, toggle behavior or touch-to-command paths;
- UI redesign, layout, typography, colors or animation changes;
- generic raw-ID editor or permanent unrestricted provisioning endpoint;
- raw Homey identifiers in Git, UI, logs, evidence or diagnostics;
- OAuth, account, selected-Homey or credential architecture changes;
- Package 3B;
- Patch 013 runtime closure;
- a separate Patch 015 finalization patch;
- automatic serial capture after flash or any runtime gate that may toggle
  DTR/RTS during display startup.

## Evidence classes

- baseline and scope documentation: required;
- static validation and secrets/privacy scan: required;
- focused host tests: required;
- ESP-IDF v6.0.1 build and size: required before flash;
- flash: separate explicit phase;
- live Homey integration and visual runtime: required on target hardware;
- Homey mutation evidence: not applicable and prohibited;
- automatic serial capture: prohibited.

Build evidence does not establish live-state or visual runtime PASS.

## Runtime completion criteria

Patch 016 is complete only after all local gates pass and target validation
shows, without automatic serial capture:

1. the first configured light changes from `Släckt` to `Tänd` and back after an
   authoritative state change in Homey;
2. the second configured light changes independently;
3. unavailable or stale data shows `Otillgänglig`;
4. unmatched or invalid data shows `Okänd`;
5. absent private provisioning shows `Ej konfigurerad`;
6. no raw Homey identifier appears in UI, logs or evidence;
7. no touch or Homey mutation path has been introduced.

## Rollback

Revert the Patch 016 implementation normally and rebuild. If private light
mappings were provisioned during target validation, wipe or replace only the
`homey_alias_v1` mapping using the existing bounded store path. No account,
OAuth token, selected Homey or unrelated NVS state is to be erased.

## Publication state

- implementation: `APPLIED_LOCALLY`;
- commit: `NOT_RUN`;
- push: `NOT_RUN`;
- pull request: `NOT_RUN`.
<!-- PATCH_016_IMPLEMENTATION_SCOPE_BEGIN -->
## Locked implementation scope

Patch 016 implementation is locked to exactly 18 files. The original 14-file scope was expanded by exactly four files: `components/secure_bootstrap/secure_bootstrap_logic.c`, `components/secure_bootstrap/test_host/test_secure_bootstrap.c`, `components/secure_bootstrap/include/phone_provisioning.h`, and `components/secure_bootstrap/secure_bootstrap_esp.c`. The local phone portal accepts exactly two private device/capability pairs and maps them only to dashboard widget indices 4 and 5. The values are written through the existing Patch 015 `homey_alias_v1` dual-slot store and are never echoed or logged.

UI presentation is widget-specific: boolean values for widgets 4 and 5 render `Tänd`/`Släckt`; other boolean widgets retain `Aktiv`/`Inaktiv`. Existing `Otillgänglig`, `Okänd`, and `Ej konfigurerad` behavior is unchanged. Cards remain read-only and non-clickable.

Verified local gates: host tests, Wi-Fi state-machine tests, Patch 015 alias regression, static validator, secrets/privacy scan, exact-scope check, `git diff --check`, ESP-IDF v6.0.1 build, size, and app-partition fit are PASS. Flash, live runtime, commit, push and PR remain NOT_RUN. Automatic serial capture is prohibited.
<!-- PATCH_016_IMPLEMENTATION_SCOPE_END -->
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
