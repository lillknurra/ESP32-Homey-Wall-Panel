# Current State

- `STABLE_BRANCH=main`
- `STABLE_IMPLEMENTATION_MERGE=b6ba6caf68ebf89c4944ea422bf768d448371f7e`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH_016_LIVE_READ_ONLY_FAVORITE_LIGHT_STATUS`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-016-live-read-only-favorite-light-status`
- `PATCH_013=COMPLETE_MERGED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH_014=COMPLETE_MERGED`
- `PATCH_015=COMPLETE_MERGED`
- `PATCH_015_SOURCE_HEAD=2100df609f5c364ae4a688d563489f0ecb3a0c9e`
- `PATCH_015_PR=22`
- `PATCH_015_MERGE_COMMIT=b6ba6caf68ebf89c4944ea422bf768d448371f7e`
- `PATCH_015_BUILD=PASS`
- `PATCH_015_SIZE=PASS`
- `PATCH_015_FLASH=PASS`
- `PATCH_015_RUNTIME_VISUAL_GATE=PASS`
- `PATCH_016=ACTIVE_BASELINE_AND_SCOPE_DEFINED`
- `PATCH_016_BASE=b6ba6caf68ebf89c4944ea422bf768d448371f7e`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable result

Patch 015 – Private Homey Alias Provisioning and Read-Only Dashboard Activation
Foundation is complete and squash-merged to `main` through PR #22. Its source
head is `2100df609f5c364ae4a688d563489f0ecb3a0c9e` and the verified merge commit is `b6ba6caf68ebf89c4944ea422bf768d448371f7e`.
Host/static validation, ESP-IDF v6.0.1 build, size, flash and the visual runtime
gate passed. The display was stable after restart without serial logging.
Automatic serial capture after flash is prohibited because DTR/RTS reset timing
can disturb display startup.

No separate Patch 015 finalization patch is authorized or required.
Patch 013 runtime remains `NOT RUN`. Package 3B remains `NOT_STARTED`.

## Active Patch 016 scope

Patch 016 – Live Read-Only Favorite Light Status is the only active development
patch on branch `patch-016-live-read-only-favorite-light-status` from exact base `b6ba6caf68ebf89c4944ea422bf768d448371f7e`.

- widget index `4`: `light_1/on`;
- widget index `5`: `light_2/on`;
- user-facing states: `Tänd`, `Släckt`, `Otillgänglig`, `Okänd`,
  `Ej konfigurerad`;
- provisioning: bounded private path through the existing local phone portal and
  Patch 015 `homey_alias_v1` store;
- raw Homey identifiers are forbidden in Git, UI, logs, evidence and diagnostics;
- Homey mutation, clickable light cards, UI redesign, Package 3B, a separate
  Patch 015 finalization and automatic serial capture are prohibited or out of
  scope;
- firmware implementation is `APPLIED LOCALLY`; commit, push and PR are `NOT RUN`.

## Immediate next work

Lock the exact Patch 016 implementation file scope, then implement the smallest
read-only provisioning and light-status presentation change. Do not commit,
push, open a PR or flash until the applicable local validation gates pass and
the operator separately authorizes each phase.
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
