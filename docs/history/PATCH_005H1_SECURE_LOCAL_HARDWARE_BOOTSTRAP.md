# Patch 005H.1 / 005H.1B — Secure Local Hardware Bootstrap

This phase disables Athom OAuth and the Athom credential portal. It uses the official Waveshare ESP32-S3-Touch-LCD-4B BSP 2.0.0 to initialize the ST7701 display, GT911 touch controller and LVGL. A cryptographically random 12-character bootstrap code is generated with `esp_fill_random`, displayed locally as text and Wi-Fi QR data, and used as the WPA2 SoftAP password. The value is never logged, persisted by application code, accepted as a command-line argument or included unsanitized in evidence.

## Runtime closure

The code is valid only before the 600-second boundary. At the exact TTL the runtime generates a new code, reconfigures the SoftAP password, disconnects existing SoftAP clients, replaces the displayed text and QR code, and wipes the previous code. Old application and transport credentials therefore stop working after rotation.

After `IP_EVENT_STA_GOT_IP`, the runtime stops the bootstrap HTTP server, changes Wi-Fi mode from APSTA to STA, wipes the in-memory provisioning code, removes the code and QR from the display, and records that bootstrap reopening requires physical action. Losing the station connection does not automatically reopen the bootstrap channel.

The BOOT button must be held continuously for five seconds to call `esp_wifi_restore()` and restart provisioning. This is Wi-Fi reprovisioning, not a full-device factory reset. It does not call `nvs_flash_erase()` and therefore does not erase unrelated application namespaces. The NVS recovery erase in `app_main` remains limited to ESP-IDF initialization failures (`NO_FREE_PAGES` or `NEW_VERSION_FOUND`).

## Dependency model

Patch 005H.1B uses dependency model B. `dependencies.lock` is a generated, Git-ignored build artifact and is not part of the central 39-file source scope. Offline evidence includes the complete generated lockfile and SHA-256 values from two full builds; the hashes must be identical. `managed_components/` and `build/` are also generated and ignored.

Touch is initialized by the BSP and an on-screen touch-test button emits a sanitized hardware marker. No destructive action depends on touch.

No eFuse, flash encryption, NVS encryption, Athom credential, OAuth exchange, commit, push or merge is performed. Device security provisioning is deferred to Patch 005H.2.

## Patch 005H.1C — Hardware boot-capture repair

The Waveshare ESP32-S3-Touch-LCD-4B does not expose a separate RESET button for the operator workflow. Hardware instructions and the runner must not request one.

The hardware runner now supports two explicit actions:

- `FLASH`: flash the verified firmware, then capture hardware evidence;
- `CAPTURE`: reuse the firmware already flashed and repeat only boot-log and hardware evidence collection.

For boot capture, the runner opens the serial port before attempting an electronic DTR/RTS reset. If that reset does not produce the required boot markers, the only manual fallback is a controlled USB power cycle:

1. disconnect USB;
2. wait at least three seconds;
3. reconnect USB;
4. capture the new boot without reflashing.

`BOOT` remains reserved for the documented five-second physical Wi-Fi reprovision action. `PWR` is not described as RESET. A missed boot log is classified as a capture failure, not a firmware or flash failure.


## Patch 005H.1D - Serial byte decoding repair

Hardware capture reads UART output as raw bytes. The runner decodes the collected
stream with UTF-8 replacement semantics (`errors="replace"`) so boot-ROM noise,
partial multibyte sequences, or non-text bytes cannot crash the evidence workflow.
Replacement characters are retained only in the sanitized diagnostic stream.
This repair does not flash firmware or change runtime firmware behavior.

## Hardware completion record

Patch 005H.1E through 005H.1G completed the physical bootstrap validation. The
verified firmware SHA is
`c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`.
Display legibility, QR, GT911 touch, protected provisioning, IP-obtained closure,
SoftAP/HTTP shutdown, code wipe, five-second BOOT reprovisioning, fresh bootstrap
and sanitized evidence are PASS. Device security provisioning remains deferred
to Patch 005H.2 and real Athom OAuth remains NOT RUN.
