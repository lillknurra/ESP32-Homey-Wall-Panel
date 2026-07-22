# Memory Configuration

## Physical capacity

- Flash: 16 MB, `VERIFIED_PRIMARY`.
- PSRAM: 8 MB, `VERIFIED_PRIMARY`.

## Official exact-board ESP-IDF example settings

The UI-oriented examples in `ESP-IDF-v5.4.2` select:

- `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y`;
- `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`;
- `CONFIG_SPIRAM=y`;
- `CONFIG_SPIRAM_MODE_OCT=y`;
- `CONFIG_SPIRAM_SPEED_80M=y`;
- instruction fetch and read-only data from PSRAM in the LVGL example.

These are `EXAMPLE_DEPENDENT`: strong candidates for ESP-IDF v6.0.1 bring-up, but not yet project configuration.

## Excluded generic configuration

`ESP-IDF-v5.4.2/01_AXP2101` and the bundled XPowersLib ESP-IDF example use generic settings such as 2 MB flash, DIO, 40 MHz, and no PSRAM. Those files demonstrate the PMIC library and are not valid board memory evidence.

## Unresolved items

- authoritative flash frequency for the project: `UNVERIFIED`;
- ESP-IDF v6.0.1 option names and compatibility: `UNVERIFIED` until a dedicated configuration patch;
- boot and runtime stability on physical hardware: not `HARDWARE_VALIDATED`.

Patch 002 does not change `sdkconfig.defaults`.
