# Patch116 - OTA foundation: OFFLINE / NOT INSTALLABLE

Baseline: `main` commit `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`,
merge tree `cd482e7020c83e005ab3fbd5e78e2f67c22131e7`.

The currently installed, physically validated app is Patch113 appversion `6ae3516`.
It controls Solskydd and Balkongmarkis with production write-arm defaults 1/1;
Markis remains disabled. Preserve the 64-byte data cache-line, 12 MHz PCLK,
20-row RGB bounce buffer and existing LVGL rendering configuration.

**STATUS: partition geometry and pure host policy model tested. No ESP-IDF
build, signed image, OTA transport, bootloader, partition write, device
migration or rollback has been implemented or validated in this package.**

## Scope and non-goals

This candidate adds *only* an opt-in sample CSV, an offline layout validator,
a pure decision-model C source/header, mock host regression and this document.
Nothing is wired into ESP-IDF `CMakeLists.txt`, there is no OTA endpoint, and
`partitions.csv` or `sdkconfig` are **not modified**.

The current app has only a factory partition and no otadata. OTA is not possible
as a safe A/B update on that installed layout. OTA implementation and migration
are gated separately from this foundation. Wi-Fi/Homey credentials, aliases,
NVS, display, flash journal and all Homey writes are out of scope.

## Candidate partition map

All ends are exclusive; 16 MiB flash ends at `0x1000000`.

| Name | Type/subtype | Offset | Size | End |
| --- | --- | ---: | ---: | ---: |
| nvs | data/nvs | 0x009000 | 0x006000 | 0x00F000 |
| phy_init | data/phy | 0x00F000 | 0x001000 | 0x010000 |
| factory | app/factory | 0x010000 | 0x600000 | 0x610000 |
| diaglog | data/undefined | 0x610000 | 0x100000 | 0x710000 |
| otadata | data/ota | 0x710000 | 0x002000 | 0x712000 |
| gap (unallocated) | - | 0x712000 | 0x00E000 | 0x720000 |
| ota_0 | app/ota_0 | 0x720000 | 0x400000 | 0xB20000 |
| ota_1 | app/ota_1 | 0xB20000 | 0x400000 | 0xF20000 |
| tail (unallocated) | - | 0xF20000 | 0x0E0000 | 0x1000000 |

Bootloader and partition table occupy the existing reserved region below
`0x9000`; partition table remains at `0x8000` (4 KiB sector). The 4 MiB OTA
slots are 64 KiB aligned. `otadata` is 8 KiB, as required by IDF.

Current Patch113 image size is 1,749,184 bytes; it fits either OTA slot *as an
unsigned size check only*. The signed/rebuilt image MUST be measured again.
There is 917,504 bytes of unallocated tail space.

**Do not activate this CSV in a production build yet.** Static geometry does
not prove that the rebuilt rollback + app-signing bootloader fits before
`0x8000`, nor that a migration will preserve recovery ability.

## Security and initial signing migration

Current `sdkconfig` is:

- `# CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is not set`
- `# CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT is not set`
- `# CONFIG_SECURE_BOOT is not set`
- `CONFIG_PARTITION_TABLE_OFFSET=0x8000`
- `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`

ESP-IDF 6.0.1's signed-app verification without hardware Secure Boot uses the
first signature/public key block in the running app as its update trust anchor.
**The initial firmware must therefore be signed too**, with one valid
signature block. The existing unsigned Patch113 factory app cannot simply be
used as the trust anchor. A compatible signed initial/factory image and key
provisioning strategy must be proven before any OTA mechanism is enabled.

Evaluate `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT` and
`CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT` with RSA-3072 app signing.
Keep the private signing key offline, outside all repositories and artifacts.
Do not enable hardware `CONFIG_SECURE_BOOT`, flash encryption, anti-rollback
fuses or eFuse modifications. Check if enabling signing changes bootloader
size or factory validation, and if it forces changing the partition-table
offset; if so, STOP and revisit the layout rather than shifting NVS.

Use a controlled authenticated HTTPS release source, an allowlisted/verified
firmware origin, trusted signed image checks and authenticated metadata for
image digest, target board and version. A SHA-256 digest obtained solely from
the same untrusted source does not authenticate firmware.

Do NOT expose an unauthenticated LAN HTTP POST or accept arbitrary update URLs.
The current provisioning HTTP server must not become an automatic update API.
Require interactive, authenticated opt-in and separate explicit reboot consent.
A version/downgrade policy should be defined before production rollout.

## OTA state machine (future device integration)

1. IDLE: accept no automatic releases/requests.
2. Authenticated admin explicitly selects signed release and confirms update.
3. Check destination is inactive `ota_0` or `ota_1`, OTA partition available,
   signed initial app trust anchor present, no running app PENDING_VERIFY.
4. Ensure no Homey write/awning command in progress; acquire bounded update
   exclusivity and verified RGB-flash maintenance barrier. Stop new commands.
5. Begin a bounded HTTPS download (no credentials exposed). Enforce `Content-
   Length` or equivalent bounded byte-count and signed image size <= target slot.
6. Use ESP-IDF native `esp_ota_begin`/`write`/`end` with validated image and
   RSA signature verification; verify signed authenticated release metadata.
   Reject missing/truncated/corrupt/unauthorized/incorrect-chip images.
7. Only after complete verification can `esp_ota_set_boot_partition` select
   the new OTA app. Explicitly confirm reboot as a separate operator action.
8. Next boot `ESP_OTA_IMG_PENDING_VERIFY`: run bounded **local** health checks
   including NVS readability, core services and display readiness; do NOT wait
   for Homey READY (intermittent Homey network has known unrelated failures).
   Ensure safe flash/otadata write conditions before calling
   `esp_ota_mark_app_valid_cancel_rollback`.
9. If app fails to start or self-test fails, rely on IDF rollback; do not call
   valid. If explicitly initiating rollback, first verify rollback candidate is
   available and avoid a reboot loop.
10. Report outcome via read-only, sanitized local diagnostics. No Homey write
    or markis movement is part of update, boot test or rollback.

The included `panel_ota_policy.c` is a **pure, unwired decision model**. Its
boolean inputs are NOT an authentication, TLS, signature or hardware proof.
The integrating caller must obtain all trusted evidence from actual ESP-IDF/
security subsystems. Until that integration is validated, it MUST NOT be used
to trigger any flash action.

## RGB and flash write safety

Patch109 pauses persistent diagnostic-journal writes during active display
transfers to prevent RGB corruption. OTA writes are separate flash writes,
so that barrier by itself does **not** establish OTA safety.

The firmware implementer must audit the exact display/BSP DMA stop mechanism,
other flash-writing tasks, PSRAM framebuffer traffic, NVS changes and power
failure behavior under ESP-IDF 6.0.1. It must build a verified *exclusive*
maintenance mode with start/stop acknowledgment and a bounded failure path.
Never force-flash while RGB DMA is active merely because a static overlay is
shown. The boot-time otadata validity write also needs flash safety.

## One-time migration: required gates, NOT instructions to execute now

1. Re-verify current Git main, exact factory image, partition table, bootloader
   binary and ESP-IDF 6.0.1. Confirm ESP32-S3 N16R8 and 16 MB flash physically
   when serial operations are independently authorized.
2. Generate and safeguard signing key offline on the operator's Mac. Never copy
   the private key into this ChatGPT container, GitHub, or shared evidence.
3. Configure and build a **signed factory app** with new OTA layout and genuine
   IDF rollback enabled. Confirm source-equivalence of Homey writes, display
   and 1/1 default arming, plus signed app layout and factory size.
4. Build and check new bootloader size < 0x8000; verify the exact outputs using
   ESP-IDF `gen_esp32part.py` and `idf.py partition-table`. Stop if the table
   offset must change or the factory image is not valid/signed.
5. Make a private local backup of sensitive NVS and other needed data according
   to explicit operator approval; do not upload credentials. Check `otadata`
   new area is erased/usable, rather than assuming it is all 0xFF.
6. Produce a **strictly bounded migration write plan** (bootloader, table,
   signed factory image and, only if needed, exact `otadata` initialization),
   with absolute offsets, hashes, no `erase-flash`, no NVS or diaglog writes.
7. Explicitly explain that bootloader/partition table migration is NOT
   power-fail-safe OTA. Have a tested serial recovery procedure and stable
   power before operator consent.
8. Operator must independently authorize exact physical writes. Abort on
   unexpected chip, offsets, partition capacity, flash size or hash results.
9. Verify local UI/display/Homey and all preserved NVS/aliases after migration.
   Verify initial boot uses factory as expected when otadata is empty.
10. Only later test OTA with a signed test image, confirm first-boot health and
    rollback; a second OTA tests alternating slot selection.

No part of this plan authorizes USB, NVS, Homey, eFuse, OTA or flash operations.

## Verification and repeatability

Run in the repo root after applying *only* the additive Patch116 files:

```sh
python3 scripts/validate_patch116_ota_partitions.py --image-bytes 1749184
python3 -m unittest discover -s tests/patch116 -p 'test_*.py' -v
python3 scripts/run_patch116_ota_policy_tests.py
```

To verify candidate table using actual local IDF, use the exact installed
ESP-IDF 6.0.1 `gen_esp32part.py` script and check its CLI flags. This has **not
been run in the current environment**. Build with OTA table only in a separate
opt-in worktree after completing bootloader and signing migration review.

Official references (specific to ESP-IDF v6.0.1 and ESP32-S3):

- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-guides/partition-tables.html
- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/system/ota.html
- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/security/secure-boot-v2.html
- https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/api-reference/kconfig-reference.html

## Follow-on work (no operations authorized)

The next bounded verification step is an **offline-only ESP-IDF 6.0.1 native
generator/bootloader/signing feasibility audit** on the operator's Mac. Do not
flash until that audit is PASS and independently authorized. After OTA comes
configurable shared widget/page data and a mobile web client; neither feature
is implemented in Patch116.
