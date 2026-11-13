# Patch120B offline implementation and migration plan

Status: offline candidate only. Nothing in this document authorizes flashing,
partition migration, OTA, serial access, or publication.

## Implemented flow

The normal dashboard has a local Settings action. It reserves the existing
Homey/network and awning command gates only when workers and queues are idle,
then records a one-boot stable-update intent in RTC memory and restarts. The
intent contains no URL, credential, Homey identifier, or release choice. It is
accepted only for the expected software reset and is cleared before networking.
Its CRC detects accidental corruption; it is not authentication.

The maintenance boot initializes the existing NVS startup path, starts a
saved-Wi-Fi STA connection, synchronizes wall time, fetches the fixed HTTPS
stable manifest, and stages only the inactive OTA application slot. RGB/LVGL,
Homey inventory, and command workers do not start.
The OTA writer never selects NVS as a target and the update flow issues no NVS
set/erase API calls. However, the existing `nvs_flash_init()` startup path is
not a strict read-only mount: ESP-IDF v6.0.1 can perform internal page recovery
writes/erases after an interrupted NVS operation. Therefore this candidate
cannot guarantee byte-for-byte NVS immutability during maintenance startup; it
reuses the same NVS initialization semantics as normal firmware startup.
The transfer uses bounded
metadata, a single owned HTTP client, disabled redirects, a fixed same-origin
release URL, exact content length, image preamble checks, a streaming SHA-256,
and ESP-IDF `esp_ota_end()` signed-image validation. `otadata` is changed only
after the complete image passes those checks. Success and failure both restart
once; the consumed intent is not rearmed.

The supervisor can request a whole-device restart after 15 minutes but does
not delete the task owning TLS. The synchronous ESP-IDF DNS/TLS/HTTP APIs may
block beyond application-side deadlines; this is not a hard network deadline.
If the restart interrupts an incomplete write, only the inactive OTA slot may
be partial; boot selection is not requested until validation succeeds.

The supervisor defers restart while a native `esp_ota_*` operation is active
and only resolves an admitted boot-selection result after that API returns. A
native flash or boot-selection call that never returns therefore has no
software recovery bound; the implementation fails closed instead of resetting
across the flash operation. Successful boot selection records `STAGED` and the
maintenance worker restarts immediately before HTTP/TLS cleanup. Failed
selection releases the flash gate and leaves the 15-minute timeout recovery
available if later client cleanup stalls.

The first boot of an OTA slot uses the existing local rollback gate before
display or Homey startup. It checks partition/image identity, read-only NVS
namespace access, PSRAM initialization, and local heap thresholds, then uses
ESP-IDF rollback APIs. These checks do not demonstrate visible display health,
Homey connectivity, actuator operation, or long-term reliability. ESP-IDF
v6.0.1 selects the factory application as rollback fallback when the prior OTA
selection record is invalid and the factory image validates; after an OTA slot
has been confirmed, the previous valid OTA slot is the normal rollback target.
This behavior is source-verified, not hardware-verified here.

## Partition migration

The proposed table preserves the existing NVS, PHY-init, factory, and
diagnostic-log geometry and adds OTA metadata and two 4 MiB slots:

| Name | Offset | Size | Use |
|---|---:|---:|---|
| nvs | `0x9000` | `0x6000` | Existing settings and saved Wi-Fi configuration |
| phy_init | `0xF000` | `0x1000` | Existing PHY data |
| factory | `0x10000` | `0x600000` | Signed factory recovery application |
| diaglog | `0x610000` | `0x100000` | Existing diagnostics |
| otadata | `0x710000` | `0x2000` | ESP-IDF OTA selection/state records |
| ota_0 | `0x720000` | `0x400000` | First OTA slot |
| ota_1 | `0xB20000` | `0x400000` | Second OTA slot |

The physical P4N16 partition table has not been read back. A one-time USB
migration is therefore required before OTA can be enabled. A future separately
authorized migration must independently back up and verify the existing NVS
and diagnostic partitions before writing anything; backups contain private
provisioning data and must remain encrypted and local, never enter source
control or shared evidence. Do not use `erase_flash` or erase NVS.

The following are command shapes for a future reviewed operator procedure,
not an executable runner and not authorized for this task. The operator must
select one port explicitly and verify the ESP32-S3 before any read or write.
First read the bootloader, partition table, current factory extent, NVS, and
diagnostic partition, using the actual partition table to confirm the extents:

```text
python -m esptool --chip esp32s3 --port <approved-port> read-flash 0x000000 0x008000 pre_migration_bootloader.bin
python -m esptool --chip esp32s3 --port <approved-port> read-flash 0x008000 0x001000 pre_migration_partition_table.bin
python -m esptool --chip esp32s3 --port <approved-port> read-flash 0x009000 0x006000 pre_migration_nvs.bin
python -m esptool --chip esp32s3 --port <approved-port> read-flash 0x610000 0x100000 pre_migration_diaglog.bin
python -m esptool --chip esp32s3 --port <approved-port> read-flash 0x010000 0x600000 pre_migration_factory_extent.bin
```

The factory read is the current 6 MiB partition extent, not a directly
flashable application BIN. Keep these backups outside the repository in an
encrypted, access-controlled location; verify their lengths and SHA-256 and
retain them until the migration has passed independent read-back. If the live
table does not match the expected old extents above, stop and revise the plan
before reading data ranges or writing flash.

After a separate authorization and backup review, the proposed write/verify
sequence is bootloader `0x000000`, partition table `0x008000`, signed factory
application `0x010000`, and the generated blank OTA data image `0x710000`,
each with exact-file hashes recorded. The NVS and diagnostic partitions are
not write targets. Independently read the four written ranges back and compare
hashes to their approved source artifacts. Verify the signed factory image
with ESP-IDF `espsecure` and parse the read-back partition table. Do not include
`erase_flash`, implicit reset loops, a second port, retries, or automatic
rollback.

If migration fails, stop and preserve logs. A separately reviewed recovery
must use the saved original bootloader, partition table, factory extent, NVS,
and diagnostic bytes. Restore only the recorded original ranges in a deliberate
sequence that first restores the matching bootloader and partition table,
then the original factory extent and saved data partitions; independently read
back and hash each range. If the failure phase or original table identity is
uncertain, do not attempt restoration until the recovery plan is reviewed.
ROM download mode and a verified serial procedure are required for recovery if
the new bootloader or partition table prevents normal application startup.

Required migration artifacts and locations are: ESP-IDF v6.0.1 bootloader at
`0x0`, the reviewed partition table at `0x8000`, the production-signed factory
application at `0x10000`, and the generated blank `ota_data_initial.bin` at
`0x710000`. ESP-IDF v6.0.1 generates the blank OTA selection image from the
configured 0x2000-byte `otadata` partition; it starts erased so the bootloader
uses the factory image. Never substitute an arbitrary `otadata` dump. The
future migration plan must explicitly preserve the NVS range `0x9000..0xEFFF`
and diagnostic range `0x610000..0x70FFFF`, and independently verify the
partition table and all application images after writes. No executable
physical migration runner is included in this candidate.

If any write or verification phase fails, stop without automatic retry,
rollback, or erase. Recovery requires a separately approved, reviewed serial
procedure and known-good signed factory artifacts. The exact recovery action
must be chosen from the captured phase evidence; do not improvise offsets.

## Signing and release process

ESP-IDF v6.0.1 `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT` and
`CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT` make OTA signature verification
use the key carried by the currently running signed image. Therefore the
production-signed factory image is the initial trust source; OTA images must be
signed with the corresponding authorized production key. The SDK startup check
rejects a running image without a signature block when signed-on-update is
enabled. This is a software trust chain, not hardware Secure Boot: a person
with physical flash access can replace firmware and its software trust source.
No eFuse, Secure Boot, flash-encryption, or hardware anti-rollback setting is
enabled here.

The local test build uses a disposable test key outside the repository. Its
signed image is for offline integration verification only and is not suitable
for installation or release. No production signing key is available or
included. The publisher requires an explicit public key and ESP-IDF signature
verification but does not sign, upload, or publish. GitHub Pages hosting has
not been configured or verified, and no manifest or image has been published.

The release sequence is a compiled software policy value, not an eFuse counter.
It is deterministically derived from the semantic version authenticated in the
signed ESP application descriptor as `major * 1,000,000 + minor * 1,000 +
patch`; minor and patch must each be in `0..999`. The current `1.0.0` image
therefore compiles sequence `1,000,000`. The manifest parser rejects a sequence
that does not match its version; the release publisher verifies the signed
image and checks its project, ESP32-S3 target, version, and derived sequence;
the running installer checks the compiled sequence against the running signed
descriptor and the candidate descriptor against the manifest before allowing
boot selection. The image digest and ESP-IDF native signature validation remain
mandatory. This software policy does not enable eFuse anti-rollback and does
not prevent an attacker with physical rewrite access from installing older
firmware or replacing the software trust source.

## Remaining activation gates

- Obtain and protect the production RSA-3072 signing key; validate an approved
  signed factory image and an OTA image against the same trusted key.
- Verify GitHub Pages deployment, immutable versioned artifacts, TLS, and the
  stable manifest location without publishing from this task.
- Independently inspect the installed device partition table and plan the
  one-time USB migration with protected NVS backup and restoration evidence.
- Validate Wi-Fi, time synchronization, certificate checks, OTA staging,
  signature rejection, first-boot confirmation, and rollback on representative
  ESP32-S3 hardware under separate authorization.
- Measure actual task-stack high-water, internal/PSRAM margins, and the
  supervisor/restart behavior on hardware. The Kconfig heap minima and task
  stack sizes in this candidate are configured values, not physical
  measurements.
- The installed P4N16 firmware is not Patch120A and has no OTA partition
  layout. The durable awning guard remains physically UNKNOWN.
