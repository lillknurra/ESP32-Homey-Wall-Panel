# Touch Interface

## Controller and bus

- Controller: GT911, `VERIFIED_PRIMARY`.
- Control bus: common I2C on GPIO47/GPIO48, `VERIFIED_PRIMARY`.
- Coordinate extent used by the BSP: 480 x 480, `EXAMPLE_DEPENDENT`.
- BSP transform flags: no XY swap and no X/Y mirror, `EXAMPLE_DEPENDENT`.

## Reset and interrupt/address-selection

The exact-board BSP does not assign direct ESP32 GPIOs for touch reset or interrupt. Instead it manipulates TCA9554 lines during display creation:

1. EXIO6 output low;
2. EXIO5 output low;
3. delay;
4. EXIO5 high;
5. delay;
6. EXIO6 changed to input.

Joining this sequence to schematic nets yields:

- EXIO5 = GT911 reset, `DERIVED_PRIMARY`;
- EXIO6 = GT911 interrupt/address-selection, `DERIVED_PRIMARY`.

## Address status

The BSP references the GT911 driver's address macro rather than a numeric literal. Patch 002 does not freeze the expanded numeric value or claim which address the physical controller selects after the strap sequence. Status: `UNVERIFIED`.

A later touch bring-up patch must log:

- exact dependency version and expanded address constant;
- I2C probe result;
- reset/address-select sequence;
- controller identity registers;
- coordinate range, orientation, and edge behavior.
