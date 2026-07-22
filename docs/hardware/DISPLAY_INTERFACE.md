# Display Interface

## Architecture

The ST7701 panel uses two interfaces:

- 16-bit parallel RGB for continuous pixel transfer;
- 3-wire SPI through TCA9554 for panel commands.

This split is `VERIFIED_PRIMARY` from the schematic and pinned BSP.

## Resolution and pixel format

- Native resolution: 480 x 480, `VERIFIED_PRIMARY`.
- BSP framebuffer format: RGB565, `EXAMPLE_DEPENDENT` until hardware execution.
- Framebuffer placement in PSRAM in the BSP: `EXAMPLE_DEPENDENT`.

## Competing official timing sets

| Source | PCLK | H front | H pulse | H back | V front | V pulse | V back | Class |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| Pinned Waveshare BSP using Espressif macro | 16 MHz | 20 | 10 | 10 | 10 | 10 | 10 | `EXAMPLE_DEPENDENT` |
| Exact-board Arduino examples | captured as 16 MHz-class board setup | 10 | 8 | 50 | 10 | 8 | 20 | `EXAMPLE_DEPENDENT` |

The difference is a real official-source conflict. Patch 002 does not choose a winner. A later display bring-up patch must implement one version at a time, preserve serial evidence, and test stability, orientation, color order, tearing, and long-duration operation.

## Initialization commands

The pinned BSP command table is the accepted implementation candidate. It contains:

- sleep-out with delay;
- vendor command-page selection;
- power and gamma values;
- pixel-format command;
- display inversion;
- display-on.

The byte sequence is intentionally not duplicated here. Future implementation must copy from `WS-BSP-C-c96f345` and record the component version and source checksum.

## Backlight

GPIO4 drives the board backlight control circuit. The BSP implements 5 kHz, 10-bit PWM and reverses the requested percentage before writing duty. The pin is `VERIFIED_PRIMARY`; PWM frequency/resolution are `EXAMPLE_DEPENDENT`; active-low behavior is `DERIVED_PRIMARY`.
