# Patch116E — signed HTTPS OTA staging engine (DEFAULT OFF)

**Task**: PATCH116E_SIGNED_HTTPS_STAGE_OFFLINE_V1
**Base**: Patch116 + Patch116B + Patch116D on published `main` `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`
**User decision**: OTA first-boot acceptance after local tests before LVGL/RGB. This is a conscious limited automatic-rollback policy, not authorization to flash.

## Source implementation

Adds `main/panel_ota_https_stage.c/.h`, registers the new file and ESP-IDF components in `main/CMakeLists.txt`, with isolated host tests. `main/main.c`, the existing Patch116D first-boot gate, panel UI, Homey logic, awning controls, existing `partitions.csv`, `sdkconfig` and `sdkconfig.defaults` remain unchanged.

The **default is `PANEL_OTA_HTTPS_STAGE_ENABLED=0`**. The resulting function returns `PANEL_OTA_STAGE_DISABLED` and neither includes nor calls ESP-IDF network/flash APIs. There is **no caller or OTA HTTP endpoint**. No user or network traffic can trigger an update with this patch.

When *separately enabled in a future authorized release*, `panel_ota_https_stage()` can erase/write the designated inactive OTA app slot and `esp_https_ota_finish()` can change `otadata`, i.e. it is a privileged flash writer. Do not enable in a normal build merely because its code compiles.

### Future-enabled requirements

- Compile-time ESP32-S3, rollback bootloader, signed app OTA verification without hardware Secure Boot, ESP-IDF certificate bundle, enabled Patch116D first-boot gate, fixed HTTPS URL, 4-MiB-or-smaller image limit and bounded overall transfer time. Hardware Secure Boot, eFuse anti-rollback and flash encryption are out of scope.
- **A proved, fresh authenticated administrator action**, separate from the RTC mode hint. Source-defined authorization callbacks are not authentication by themselves and **no such control plane exists yet**.
- Maintenance mode with a proven RGB/LVGL-not-started condition and no running Homey/awning command tasks. Future integration must supply these facts from a verifiable control plane; booleans/callbacks are an interface contract, not an authenticity guarantee.
- A public, approved update host reached through verified TLS CA roots. The URL is an immutable build-time string, not user-supplied; no HTTP, non-default port, query string, embedded credentials or automatic redirects are accepted. `updates.example.org` in the host tests is a **fixture, not an approved production release host**.
- A known, signed running factory/OTA application; a release signing key kept offline and outside Git; signed application images using the same trust chain. This patch neither creates nor uses a real signing key.
- A **release procedure that increases `esp_app_desc.secure_version` on each signed release**. The staging engine rejects a candidate with non-increasing `secure_version` or the same app version. This is a software-only version policy, not hardware/eFuse anti-rollback.

### Bounded flow

1. Recheck callbacks and complete expected partition geometry; accept only factory or a `VALID` OTA current app, and an **inactive** exact `ota_0`/`ota_1` slot.
2. Verify the currently executing image descriptor matches the flashed running partition and the project identity.
3. Open immutable HTTPS URL with `esp_crt_bundle_attach`; disable redirects and HTTP auth retries.
4. Before any OTA partition write, require a known `Content-Length` between 4096 bytes and the compile-time limit, parse the remote app descriptor, check project/name/version and increasing `secure_version`; the header is still **untrusted until signed verification succeeds**.
5. Recheck fresh authorization and maintenance state, stream via ESP-IDF v6.0.1 `esp_https_ota_perform`, enforce bounded size, monotonic byte count and elapsed-transfer deadline. On any failure or revoked authorization, **abort**, never finish or reboot.
6. Require complete declared transfer, recheck authorization, and invoke `esp_https_ota_finish`. In the signed-app update configuration, ESP-IDF performs image verification and sets the OTA boot partition only after successful verification. `finish` frees the handle; do not abort again on a `finish` failure.
7. Return `STAGED_NO_REBOOT` to the (future) caller. The release workflow requires an **additional explicit reboot authorization** and Patch116D first-boot acceptance. No reboot or Homey command is made by this module.

## Important audit blockers (NOT PRODUCTION READY)

1. ESP-IDF v6.0.1 `esp_https_ota_get_img_desc` invokes an internal header-read path; that path retries `ESP_ERR_HTTP_EAGAIN` internally, potentially beyond this adapter's *perform-loop* deadline. The `http.timeout_ms` bounds individual operations, not necessarily total time in header parsing. **Review and resolve this separately before activation**. Never assert globally bounded network transport from the host tests alone.
2. A **real authenticated admin control plane and maintenance-mode entry** do not exist. RTC handoff by itself never confers authorization. The stage function is intentionally not called.
3. The official firmware sign/verify key lifecycle, signed + rollback combined bootloader fit, signed initial factory image, and operator rescue/USB migration procedures are not finalized.
4. The feature-enabled native ESP-IDF build and actual live signed-update rejection/rollback tests have not been performed. Host tests simulate OTA APIs; they prove local logic only. No firmware, partition table or hardware mutations are authorized by this patch.
5. The source-side initial health checks do not detect every later display regression. The user accepted this specific limitation contingent on independently verified recovery.

## Offline host evidence

Run `python3 scripts/run_patch116e_https_stage_tests.py`. The test harness compiles the actual adapter with simulated ESP-IDF APIs under GCC/Clang and ASan/UBSan, and verifies the default-off artifact and compile-time fail-closed configuration. It cannot validate signed real images, certificate chains, ESP-IDF scheduler behavior, bootloader rollback, physical hardware, true authorization or externally triggered updates.

## Change and operation boundaries

No Git commit, push, PR, merge, hardware flash, OTA request, serial operation, NVS change, diaglog erase, Homey communication or awning motion was performed. Source generated in a separate environment. Keep installed Patch113 untouched.
