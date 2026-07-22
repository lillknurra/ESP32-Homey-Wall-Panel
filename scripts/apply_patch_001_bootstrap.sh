#!/usr/bin/env bash
set -euo pipefail

EXPECTED_REPO="ESP32-Homey-Wall-Panel"
EXPECTED_BRANCH="agent/bootstrap-project"
BASE_COMMIT="64cc6592c9c6f756c4a068cef51b26df7faf3c33"

fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

repo_root="$(git rev-parse --show-toplevel 2>/dev/null)" || fail "Run this inside the Git repository."
cd "$repo_root"
[[ "$(basename "$repo_root")" == "$EXPECTED_REPO" ]] || fail "Expected repo $EXPECTED_REPO, got $(basename "$repo_root")."
[[ "$(git branch --show-current)" == "$EXPECTED_BRANCH" ]] || fail "Expected branch $EXPECTED_BRANCH."
[[ -z "$(git status --porcelain)" ]] || fail "Worktree is not clean. Commit/stash/remove changes first."

mkdir -p main prompts docs/handoff docs/architecture docs/development docs/history docs/hardware scripts

cat > CMakeLists.txt <<'EOT'
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_homey_wall_panel)
EOT

cat > main/CMakeLists.txt <<'EOT'
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")
EOT

cat > main/main.c <<'EOT'
#include "esp_log.h"
#include "esp_system.h"
#include "esp_idf_version.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Homey Wall Panel bootstrap");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
}
EOT

cat > sdkconfig.defaults <<'EOT'
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_COMPILER_OPTIMIZATION_DEBUG=y
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
EOT

cat > .gitignore <<'EOT'
# ESP-IDF build output
build/
sdkconfig
sdkconfig.old
dependencies.lock
managed_components/

# IDE and OS
.vscode/settings.json
.idea/
.DS_Store

# Local evidence and temporary files
artifacts/
*.log
*.tmp
EOT

cat > README.md <<'EOT'
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
EOT

cat > AGENTS.md <<'EOT'
# Agent Instructions

This repository uses conservative, evidence-driven, patch-based engineering.
Preserve verified behavior and prefer small, traceable, reversible changes.

## Source of truth

- The Git repository is authoritative.
- Documentation records intent and durable project knowledge.
- Validation evidence determines correctness.
- Chat history is not a substitute for repository state.

## Read first

Read:

1. `prompts/START_PROMPT.md`
2. `docs/handoff/MASTER_INDEX.md`

Then follow the official reading order in `MASTER_INDEX.md`.

## Documentation ownership

- `AGENTS.md` owns permanent AI and repository behavior.
- `PROJECT_INSTRUCTIONS.md` owns permanent project-specific constraints.
- `docs/handoff/MASTER_INDEX.md` owns the official reading order.
- `docs/handoff/CURRENT_STATE.md` is the sole authority for active branch, baseline, patch status, evidence status, and immediate next work.
- `docs/handoff/HANDOFF.md` carries active work context between sessions.
- `docs/history/` records completed and historical work.

## Patch workflow

```text
BASELINE AUDIT -> IMPLEMENT -> VALIDATE -> PUBLISH -> REMOTE VERIFY -> LOCK -> COMPLETE
```

Never promote `FAIL`, `INCONCLUSIVE`, or `NOT RUN` to `PASS`.

## Repository write policy

For normal code, tooling, test, and documentation changes:

1. edit locally;
2. validate exact local files;
3. inspect the complete diff;
4. commit locally;
5. push the commit;
6. verify the remote ref.

Use GitHub connectors primarily for repository state, PR metadata, comments, reviews, and small administrative changes.

## Git safety

- One logical purpose per patch and commit.
- Stage exact paths only.
- Inspect status, staged names, diff statistics, and full staged diff.
- Do not rewrite published history without explicit authorization.
- Do not claim completion before remote verification.

## Validation and communication

- Claims must match the evidence class actually validated.
- State assumptions, risks, unknowns, and validation status explicitly.
- Provide copy/paste commands for user-run validation.
- Preserve failed evidence.
- Provide one exact next action and a complete continuation prompt.
EOT

cat > PROJECT_INSTRUCTIONS.md <<'EOT'
# Project Instructions

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Local path: `~/GitHub/ESP32-Homey-Wall-Panel`
- Default branch: `main`
- Primary target: `ESP32-S3 using ESP-IDF`
- ESP-IDF baseline: `v6.0.1`
- ESP-IDF path: `~/GitHub/esp-idf-v6.0.1`

Every patch defines purpose, non-goals, base branch and commit, expected files,
design decisions, validation, completion criteria, rollback, and next work.

Keep evidence classes separate: documentation, build, runtime, integration,
protocol, firmware, hardware, synchronization, packaging, and measurement.

## Project-specific principles

- The hardware baseline is Waveshare ESP32-S3-Touch-LCD-4B with ESP32-S3-WROOM-1-N16R8.
- Do not guess GPIO mappings, bus timing, peripheral addresses, power-rail sequencing, or vendor initialization details.
- Do not select enclosure or Homey integration protocol without documented requirements and evidence.
- Keep secrets, Wi-Fi credentials, Homey tokens, and device-specific provisioning data out of Git.
- Prefer ESP-IDF-native components and explicit versioning.
- Treat a successful build as build evidence only, not runtime or hardware evidence.
EOT

cat > prompts/START_PROMPT.md <<'EOT'
# Start Prompt

Continue work in `lillknurra/ESP32-Homey-Wall-Panel` from the active branch.

Read first:

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `docs/handoff/MASTER_INDEX.md`

Follow the complete reading order in `MASTER_INDEX.md`, audit the local and remote baseline, and continue only the patch named in `docs/handoff/CURRENT_STATE.md`.

Use local Git for multi-file changes. Keep changes narrow, deterministic, validated, reversible, and documented. Do not claim runtime, hardware, protocol, or integration PASS from build evidence alone.
EOT

cat > docs/handoff/MASTER_INDEX.md <<'EOT'
# Master Index

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `docs/handoff/CURRENT_STATE.md`
4. `docs/handoff/AI_MEMORY.md`
5. `docs/handoff/HANDOFF.md`
6. `docs/architecture/DESIGN_DECISIONS.md`
7. `docs/development/DEVELOPMENT_WORKFLOW.md`
8. `docs/development/VALIDATION_WORKFLOW.md`
9. `docs/hardware/HARDWARE_BASELINE.md`
10. `docs/history/PATCH_HISTORY.md`
EOT

cat > docs/handoff/CURRENT_STATE.md <<EOT
# Current State

- Project: \`ESP32 Homey Wall Panel\`
- Repository: \`lillknurra/ESP32-Homey-Wall-Panel\`
- Active branch: \`agent/bootstrap-project\`
- Base branch: \`main\`
- Base commit: \`$BASE_COMMIT\`

## Current patch

**Status:** IMPLEMENTED / UNVALIDATED / UNMERGED

\`\`\`text
Patch 001 - Bootstrap
\`\`\`

## Evidence status

- Documentation: NOT RUN
- Static validation: NOT RUN
- ESP-IDF build: NOT RUN
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN

## Immediate next work

Run `scripts/validate_patch_001.sh`, then perform an ESP-IDF build.
EOT

cat > docs/handoff/AI_MEMORY.md <<'EOT'
# AI Memory

GitHub and `docs/handoff/` own durable project state.

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Target: `Waveshare ESP32-S3-Touch-LCD-4B`

Method:

- one narrow patch purpose;
- predefined commit message;
- exact-file staging;
- deterministic applicators and validators;
- evidence-based claims;
- reversible experiments.

Known local environment:

- Repository: `~/GitHub/ESP32-Homey-Wall-Panel`
- ESP-IDF baseline: `v6.0.1`
- ESP-IDF path: `~/GitHub/esp-idf-v6.0.1`
- Observed Python: `3.14.6`
EOT

cat > docs/handoff/HANDOFF.md <<EOT
# Handoff

\`\`\`text
Repository: ~/GitHub/ESP32-Homey-Wall-Panel
Active branch: agent/bootstrap-project
Base branch: main
Base commit: $BASE_COMMIT
\`\`\`

Current work:

\`\`\`text
Patch 001 - Bootstrap
Status: implemented locally; validation not yet run
\`\`\`

Next action:

Run static validation and ESP-IDF build. Preserve complete output as evidence.
EOT

cat > docs/architecture/DESIGN_DECISIONS.md <<'EOT'
# Design Decisions

## DD-001 - Firmware framework

**Status:** Accepted for bootstrap

The firmware uses ESP-IDF v6.0.1 and targets the Waveshare ESP32-S3-Touch-LCD-4B.

Rationale:

- direct access to ESP32 platform capabilities;
- explicit component and build management;
- suitable foundation for display, networking, provisioning, and device integration.

The board-level hardware baseline is defined in `docs/hardware/HARDWARE_BASELINE.md`. GPIO mappings and peripheral initialization remain intentionally unverified until extracted from Waveshare primary artifacts and validated on hardware.
EOT

cat > docs/development/DEVELOPMENT_WORKFLOW.md <<'EOT'
# Development Workflow

1. Audit branch, local HEAD, remote HEAD, worktree, and documented baseline.
2. Define one patch purpose, non-goals, expected files, validation, rollback, and commit message.
3. Implement locally using deterministic scripts when practical.
4. Run static checks and scope-appropriate validation.
5. Inspect the exact diff.
6. Stage exact files.
7. Commit and push only after accepted validation.
8. Verify remote commit and open a draft PR.
9. Lock documentation from accepted evidence.
10. Complete only after merge and merged-baseline verification.
EOT

cat > docs/development/VALIDATION_WORKFLOW.md <<'EOT'
# Validation Workflow

Evidence classes must remain distinct.

## Static validation

Checks repository structure, placeholders, shell syntax, expected files, and source-level invariants.

## Build validation

A successful `idf.py build` proves only that the checked-out source builds in the active toolchain environment.

## Runtime and hardware validation

Require execution on the intended device and captured runtime evidence. They are not part of Patch 001 unless explicitly performed.

## Evidence capture

Use non-interactive commands and preserve full output. Prefer a timestamped evidence file under an untracked `artifacts/` directory for long output.
EOT


cat > docs/hardware/HARDWARE_BASELINE.md <<'EOT'
# Hardware Baseline

## Board identity

- Board: `Waveshare ESP32-S3-Touch-LCD-4B`
- Module: `ESP32-S3-WROOM-1-N16R8`
- ESP-IDF target: `esp32s3`
- Flash capacity: `16 MB`
- PSRAM capacity: `8 MB`

## Integrated display and peripherals

The following component identities are part of the project hardware baseline:

- LCD: `ST7701`, 4-inch, 480 x 480, RGB display interface
- Capacitive touch: `GT911`, I2C control interface
- Power-management IC: `AXP2101`
- IMU: `QMI8658`
- RTC: `PCF85063`
- GPIO expander: `TCA9554`
- Audio codec: `ES8311`
- Audio ADC / microphone front end: `ES7210`

## Evidence boundary

This file records board and component identity only. It does not yet claim verified:

- GPIO assignments;
- RGB timing or pixel-clock parameters;
- ST7701 initialization commands;
- GT911 reset, interrupt, address-selection, or coordinate configuration;
- I2C bus allocation or peripheral addresses;
- AXP2101 rail configuration and power-up sequence;
- TCA9554 pin allocation;
- flash mode or frequency;
- PSRAM bus mode or frequency;
- backlight control;
- audio routing;
- runtime operation on physical hardware.

Those details must be extracted from Waveshare primary artifacts, documented in a later patch, and validated on the physical board. No GPIO value may be inferred from a similarly named board or third-party example.
EOT

cat > docs/history/PATCH_HISTORY.md <<'EOT'
# Patch History

## Patch 001 - Bootstrap

- Status: ACTIVE / UNMERGED
- Branch: `agent/bootstrap-project`
- Base branch: `main`
- Base commit: `64cc6592c9c6f756c4a068cef51b26df7faf3c33`
- Purpose: establish a Project-Template-derived repository structure and minimal buildable ESP-IDF application.
- Non-goals: GPIO mapping, display initialization, touch initialization, peripheral drivers, UI, networking behavior, provisioning, Homey integration, enclosure, and production configuration.
- Intended commit: `chore: bootstrap ESP32 Homey wall panel project`
- Validation: static validator plus clean ESP-IDF build.
- Completion: remote branch and draft PR point to the validated commit; merge is separately accepted.
EOT

cat > scripts/validate_patch_001.sh <<'EOT'
#!/usr/bin/env bash
set -euo pipefail

root="$(git rev-parse --show-toplevel 2>/dev/null)"
cd "$root"

expected=(
  CMakeLists.txt
  main/CMakeLists.txt
  main/main.c
  sdkconfig.defaults
  README.md
  AGENTS.md
  PROJECT_INSTRUCTIONS.md
  prompts/START_PROMPT.md
  docs/handoff/MASTER_INDEX.md
  docs/handoff/CURRENT_STATE.md
  docs/handoff/AI_MEMORY.md
  docs/handoff/HANDOFF.md
  docs/architecture/DESIGN_DECISIONS.md
  docs/development/DEVELOPMENT_WORKFLOW.md
  docs/development/VALIDATION_WORKFLOW.md
  docs/hardware/HARDWARE_BASELINE.md
  docs/history/PATCH_HISTORY.md
)

for path in "${expected[@]}"; do
  [[ -f "$path" ]] || { echo "FAIL missing: $path"; exit 1; }
done

if grep -RInE '\{\{[A-Z0-9_]+\}\}' --exclude-dir=.git .; then
  echo "FAIL unresolved template placeholders"
  exit 1
fi

bash -n scripts/validate_patch_001.sh
bash -n scripts/apply_patch_001_bootstrap.sh

grep -Fq 'project(esp32_homey_wall_panel)' CMakeLists.txt
grep -Fq 'idf_component_register' main/CMakeLists.txt
grep -Fq 'void app_main(void)' main/main.c
grep -Fq 'CONFIG_IDF_TARGET="esp32s3"' sdkconfig.defaults
grep -Fq 'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y' sdkconfig.defaults
grep -Fq 'Waveshare ESP32-S3-Touch-LCD-4B' docs/hardware/HARDWARE_BASELINE.md
grep -Fq 'ESP32-S3-WROOM-1-N16R8' docs/hardware/HARDWARE_BASELINE.md

actual_idf="$(idf.py --version 2>/dev/null || true)"
[[ "$actual_idf" == 'ESP-IDF v6.0.1' ]] || {
  echo "FAIL expected ESP-IDF v6.0.1, got: ${actual_idf:-not active}"
  exit 1
}

[[ "${IDF_PATH:-}" == */esp-idf-v6.0.1 ]] || {
  echo "FAIL expected IDF_PATH ending in /esp-idf-v6.0.1, got: ${IDF_PATH:-unset}"
  exit 1
}

echo "PASS Patch 001 static validation"
EOT

cp "$0" scripts/apply_patch_001_bootstrap.sh
chmod +x scripts/apply_patch_001_bootstrap.sh scripts/validate_patch_001.sh

printf '\nPatch 001 applied.\n'
printf 'Next: ./scripts/validate_patch_001.sh\n'
