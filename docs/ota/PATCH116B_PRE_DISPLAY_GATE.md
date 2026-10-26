# Patch116B - Early-boot OTA flash safety gate (offline candidate)

**Status: HOST MODEL PASS / NOT IN FIRMWARE / NO OTA IMPLEMENTATION.**

Project source anchor: `main` / `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`.
Patch113 operator-verified app: `6ae3516`. The existing Patch116 host
foundation is a prerequisite; no current production sources are replaced.

## Finding 1: IDF v6.0.1 RGB display cannot safely be "stopped" by dimming

The project's `display_init()` in `secure_bootstrap_esp.c` invokes
`bsp_display_start()` which creates the LVGL port and RGB panel. ESP-IDF
v6.0.1 `esp_lcd_panel_disp_on_off()` in `esp_lcd_panel_rgb.c` only changes the
display-enable GPIO; it does **not** stop RGB DMA. `esp_lcd_rgb_panel_restart()`
requests a restart at VSYNC, not a stop. The RGB driver's `rgb_panel_del()`
invokes the hardware teardown, but the version-locked Waveshare BSP 2.0.0 has
no verified public stop/teardown API and owns private panel and touch handles.

Therefore the safe default is: never begin OTA flash writes once
`bsp_display_start()` has been called in the current boot, until an exact
BSP/LVGL-native stop sequence and positive stop acknowledgement are proved.
A black screen or LVGL mutex alone is insufficient.

## Candidate approach: headless, operator-initiated maintenance boot

1. While the normal UI is running, an authenticated operator explicitly
   approves one specific allowlisted release and a subsequent restart.
2. A bounded non-secret *intent* is preserved in non-flash handoff memory
   (RTC non-init memory is a hypothesis, not yet verified for this board and
   ESP-IDF v6.0.1). The intent is **not** an authentication token; CRC only
   detects accidental corruption and does not prove legitimacy. Never store
   Homey secrets or an arbitrary URL in it.
3. After an explicitly approved software restart, the program checks the
   reset reason, consumes the one-shot intent, and enters maintenance mode
   **before `bsp_display_start()` and before any RGB/LVGL initialization**.
   If anything is missing, enter ordinary boot; do not start OTA.
4. The maintenance boot initializes only the services needed for NVS READ,
   Wi-Fi and HTTPS. It blocks Homey command ingress and verifies no commands
   are in flight. Firmware update data comes from a constrained HTTPS origin;
   the app image is verified by IDF's signed-app chain of trust.
5. With no RGB/LVGL resources started in this boot and with other flash
   writers quiesced, stage into the inactive OTA slot. Verify image bounds,
   full signed image, target board, release policy and actual image signature.
6. Only after complete verification and the previously approved update intent,
   allow `esp_ota_set_boot_partition()`; record a sanitized result and reboot.
   If staging fails, do not switch boot partition; reset to the unchanged app
   using a separately verified failure policy.
7. This **does not** resolve the new app's first-boot `PENDING_VERIFY` write.
   That write touches `otadata`. There are two distinct design options:
   - Confirm local boot health *before* starting RGB: flash-safe but will not
     automatically detect a broken display pipeline.
   - Verify visible display first, then safely stop RGB/LVGL with a proven
     BSP teardown before marking valid: more complete display acceptance,
     but the teardown is presently unverified. DO NOT silently pick one.

The module `panel_ota_pre_display_policy` is a static model of preconditions.
Its booleans are controlled by its caller and are not cryptographic evidence.
It is NOT linked into the build, offers NO HTTP endpoint, performs NO flash or
Homey operations, and does not create an OTA-ready firmware image.

## Finding 2: factory fallback in native IDF code needs testing

The previous audit correctly noted that `factory` itself does not take part in
OTA-state validity transitions. However, ESP-IDF v6.0.1 `esp_ota_ops.c`,
`esp_ota_check_rollback_is_possible()` lines 1034-1043, explicitly checks for
and validates the `factory` partition if the prior OTA-data entry is invalid.
In the same file, `esp_ota_current_ota_is_workable()` comments that rollback
may boot a previous OTA slot **or factory**. This suggests a signed, validated
factory image might serve as the fallback for the FIRST OTA. Do not require
pre-populating a second OTA slot without first verifying this exact behavior
in the native bootloader and a controlled offline simulation. Do not assume
that the firmware in `factory` can itself be rolled back through OTA status.

ESP-IDF OTA docs make clear that OTA application rollback is specifically
about OTA partitions; the fallback-to-factory case is narrower and must not
be conflated with regular OTA A/B rollback.

## Known remaining gates before native OTA implementation

- Verify the RTC intent/reset semantics, power-loss handling and replay
  protection for ESP32-S3 and exact IDF 6.0.1. If non-flash handoff cannot be
  made reliable, stop and redesign rather than writing NVS while RGB is active.
- Audit `waveshare/esp32_s3_touch_lcd_4b` at pinned **2.0.0** and
  `espressif/esp_lvgl_port` at pinned **2.9.0**, not current vendor master.
- Implement an authenticated operator trigger with explicit update and restart
  consent; public LAN HTTP is not an authentication boundary.
- Verify trusted HTTPS origin, certificates, signed-image chain, single-key
  expectation for signed-app updates without Secure Boot, target/version checks,
  and exact image lengths. No private signing key in the repository.
- Verify rollback-enabled bootloader size below the existing partition table
  at `0x8000`; use only the local pinned ESP-IDF v6.0.1.
- Determine the `PENDING_VERIFY` acceptance strategy without regressing RGB
  or pretending Homey READY is a boot health requirement.
- Keep `nvs` at 0x9000, `factory` at 0x10000, `diaglog` at 0x610000,
  and do not change current `partitions.csv` or `sdkconfig` until gated.

## Offline-only testing

```sh
python3 scripts/run_patch116b_pre_display_tests.py
```

Tests reject missing consent, stale/forged RTC-like handoff evidence, active
LVGL/RGB, unquiesced Homey/flash writers, invalid signature anchor/slot,
missing rollback, and unauthenticated HTTPS/release metadata. A host model
cannot attest to native evidence or prevent races between check and flash.

## Sources (exact versions)

- IDF v6.0.1 `components/esp_lcd/rgb/esp_lcd_panel_rgb.c` (lines 237-293,
  498-509, 781-793):
  https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_lcd/rgb/esp_lcd_panel_rgb.c
- IDF v6.0.1 `components/app_update/esp_ota_ops.c` (lines 1010-1124):
  https://github.com/espressif/esp-idf/blob/v6.0.1/components/app_update/esp_ota_ops.c
- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/system/ota.html
- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/security/secure-boot-v2.html

## Scope and prohibition

No commits, publish, merge, flash, OTA, serial, reboot, NVS/diaglog modification,
Homey read/write or awning movement is authorized by this offline package.
Installed Patch113 remains the only physically verified firmware.
