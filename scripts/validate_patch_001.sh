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
