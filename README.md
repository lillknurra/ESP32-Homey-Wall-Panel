# ESP32 Homey Wall Panel

ESP-IDF firmware for an ESP32-S3 based wall panel intended to integrate with Homey.

## Current status

Patch 001 establishes the repository, documentation, validation workflow, and a minimal buildable ESP-IDF application. Hardware selection, display stack, input devices, networking, provisioning, Homey protocol, enclosure, and power architecture remain intentionally undecided.

## Prerequisites

- macOS or Linux
- Git
- ESP-IDF v6.0.1 installed at `~/GitHub/esp-idf-v6.0.1`
- ESP-IDF v6.0.1 environment activated

## Build

```bash
cd ~/GitHub/ESP32-Homey-Wall-Panel
source ~/GitHub/esp-idf-v6.0.1/export.sh
idf.py set-target esp32s3
idf.py build
```

## Repository workflow

Read in this order:

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `prompts/START_PROMPT.md`
4. `docs/handoff/MASTER_INDEX.md`

Non-trivial work follows the patch lifecycle documented in `AGENTS.md`.
