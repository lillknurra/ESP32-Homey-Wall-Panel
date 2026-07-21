# Power and IO Expander

## TCA9554

The board uses TCA9554 on the common I2C bus. The pinned BSP selects the address-strap `000` variant. This strap selection is `VERIFIED_PRIMARY`; the numeric address is deliberately deferred until the exact dependency version is frozen.

The authoritative channel allocation is documented in `PIN_MAPPING.md`.

## AXP2101

The schematic establishes AXP2101 as the PMIC and identifies its connected rails. Those schematic nets are `VERIFIED_PRIMARY`. Exact register values, enable state, ramp order, charging policy, shutdown policy, and interrupt handling are not established as project requirements by Patch 002.

The official archive includes a dedicated `01_AXP2101` ESP-IDF example. Its printed example state and generic `sdkconfig` are library-example behavior, not a board power contract. In particular, its 2 MB flash configuration must not be imported into this project.

## Rail catalog from schematic

| Rail/output | Schematic role/value | Class |
|---|---|---|
| DCDC1 | DVDD / board supply role | `VERIFIED_PRIMARY` |
| DCDC2 | 0.9 V net | `VERIFIED_PRIMARY` |
| DCDC3 | 1.2 V or board-specific net as drawn | `VERIFIED_PRIMARY` schematic-only |
| DCDC4 | 1.8 V or board-specific net as drawn | `VERIFIED_PRIMARY` schematic-only |
| DCDC5 | not connected in the captured schematic | `VERIFIED_PRIMARY` |
| ALDO/BLDO/CPUSLDO outputs | values and destinations as drawn in `WS-SCH-4B` | `VERIFIED_PRIMARY` schematic-only |

Because the phase-2 package preserves text reports rather than the schematic PDF itself, implementation must re-open `WS-SCH-4B` before encoding any voltage or enable sequence. This document intentionally does not normalize ambiguous rail labels into firmware constants.

## Power implementation gate

No PMIC write sequence may be added until a later patch defines:

- required always-on and switchable rails;
- safe defaults and brownout behavior;
- USB-only and battery-powered operation;
- charging constraints;
- shutdown/wake behavior;
- validation measurements.
