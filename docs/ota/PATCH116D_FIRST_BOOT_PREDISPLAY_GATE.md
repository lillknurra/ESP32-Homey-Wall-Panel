# Patch116D - Native OTA first-boot gate (offline implementation)

TASK_ID: PATCH116D_OTA_FIRST_BOOT_INTEGRATED_OFFLINE_V1
Date: 2026-10-25
Base: `lillknurra/ESP32-Homey-Wall-Panel` main `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`
User policy decision: accept limited automatic rollback, confirm a new OTA app after local checks **before** RGB/LVGL startup. This is not approval for a real update or installation.

## Actual source changes

- Adds `main/panel_ota_first_boot_gate.c` and its header.
- Adds a call in `main/main.c` **after** `panel_nvs_startup_gate()` returns success, and **before** `secure_bootstrap_start()` (which calls `bsp_display_start()`).
- Adds adapter to `main/CMakeLists.txt` with explicit dependencies.
- Host tests compile the real native adapter with simulated ESP-IDF APIs, not merely a detached decision model.
- All existing `partitions.csv`, `sdkconfig`, `sdkconfig.defaults`, screen code, Homey code and awning controls remain unchanged. The `partitions_ota_candidate.csv` from Patch116 remains a separate candidate.

## Safe activation boundary

Default: `PANEL_OTA_FIRST_BOOT_ENABLED=0`. In this state `panel_ota_first_boot_gate_after_nvs()` returns true with no side effects or ESP-IDF OTA calls. No OTA flash-writing capability is shipped by this patch.

Enabling the gate in a future authorized **signed, rollback-enabled** image requires all of:

- ESP32-S3 target, `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`, `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT`, `CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT`.
- Explicit `PANEL_OTA_FIRST_BOOT_ENABLED=1` compile definition (there is no such setting in the active build).
- Operator-approved `PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES` and `PANEL_OTA_MIN_PSRAM_BLOCK_BYTES`, based on measured healthy hardware. Nonzero positive values are mandatory at compile time. Host test values are **synthetic fixtures, not approved production thresholds**.
- New verified partition table, signed factory trust anchor, combined signing+rollback bootloader, authenticated update channel, and independently verified recovery procedure. None is delivered or activated here.

When enabled and running a new OTA app in `ESP_OTA_IMG_PENDING_VERIFY`, the gate requires:

1. Exact expected NVS, PHY, factory, diaglog, otadata, OTA0 and OTA1 offsets/sizes.
2. Correct running OTA partition and image descriptor/project/version consistency.
3. Settings namespace accessible read-only; `ESP_ERR_NVS_NOT_FOUND` is allowed for a new device, actual IO errors are denied.
4. Initialized PSRAM and measured free-largest-block thresholds.
5. Valid previous application fallback as reported by `esp_ota_check_rollback_is_possible()`.

Only then does the gate call `esp_ota_mark_app_valid_cancel_rollback()` and allow unchanged display startup. This is a real write to `otadata` **when enabled and pending**, but always **before** RGB starts. A failed local health check calls the official one-shot `esp_ota_mark_app_invalid_rollback_and_reboot()` only if a valid rollback target is reported; if it returns, normal startup is blocked. A failed confirmation blocks startup with no retry. Valid OTA images and factory boot bypass confirmation; unexpected OTA states block normal startup.

These tests do **not** prove a physically signed image, that the updater authenticates releases, that the current app is trusted at boot, that display output works, or that failure recovery works on hardware. In software-signed-only mode the currently running app is not cryptographically validated at every boot; hardware flash-write adversaries are outside this trust boundary.

## Explicitly NOT implemented

- No OTA download/write endpoint, HTTPS server, signing key, RTC intent handoff, dual-slot selection, payload streaming, image verification, boot partition switching, auto-update, web admin, or phone UI.
- No firmware-build proof from this environment; Mac ESP-IDF v6.0.1 native build required.
- No claim that startup self-test detects broken screen, Wi-Fi/Homey interaction or other later regressions. Recovery and physical acceptance are separate gates.
- No Git publication, NVS modification, device operation or firmware flashing.

## Native audit before activation

The Codex/Mac follow-up must check exact ESP-IDF headers and component dependencies, build with default-off, assess early boot ordering (diagnostic journal and NVS), verify both NVS/rollback/partition checks, choose measured resource thresholds, build signed+rollback bootloader without touching private keys, and construct a separate authenticated OTA maintenance transport. Do NOT activate the gate, change `sdkconfig`/partition table for the installed panel, or flash.

## Host regression

`python3 scripts/run_patch116d_first_boot_gate_tests.py`

The harness builds with GCC and Clang, ASan/UBSan, exercises 29 paths per compiler, checks default-disabled source compiles without IDF runtime, and checks compile-time signing/rollback/memory safety guards.
