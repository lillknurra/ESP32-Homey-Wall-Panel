# Audio Architecture

## Components

- ES8311 output codec: `VERIFIED_PRIMARY`.
- ES7210 microphone ADC/front end: `VERIFIED_PRIMARY`.
- Power amplifier controlled through TCA9554 EXIO3: `VERIFIED_PRIMARY`.

## I2S routing

| Signal | GPIO | Endpoint | Class |
|---|---:|---|---|
| MCLK | GPIO5 | codecs | `VERIFIED_PRIMARY` |
| BCLK | GPIO16 | codecs | `VERIFIED_PRIMARY` |
| LRCK/WS | GPIO7 | codecs | `VERIFIED_PRIMARY` |
| DOUT | GPIO6 | ESP32-S3 to ES8311 | `VERIFIED_PRIMARY` |
| DIN | GPIO15 | ES7210 to ESP32-S3 | `VERIFIED_PRIMARY` |

## BSP behavior

The pinned BSP creates duplex standard-mode I2S with a default mono, 16-bit, 22,050 Hz configuration and enables the amplifier through EXIO3. These parameters are `EXAMPLE_DEPENDENT`, not product audio requirements.

Codec addresses, clock ratios, analog gain, microphone channel routing, speaker limits, echo behavior, and runtime operation remain `UNVERIFIED` for this project.
