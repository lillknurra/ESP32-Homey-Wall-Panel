#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat PAGER=cat LESS=FRX
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/athom-executable.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
  -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_runtime.c \
  components/athom_cloud_native/test_host/test_executable_protocol.c \
  -o "$TMP/test_executable_protocol"
"$TMP/test_executable_protocol"

bash scripts/validate_patch_005_athom_transport.sh

grep -q 'ATHOM_PROTOCOL_MAX_JSON_BYTES' components/athom_cloud_native/include/athom_protocol.h
grep -q 'esp_wifi_set_mode(WIFI_MODE_STA)' components/athom_cloud_native/src/athom_runtime.c
grep -q 'Type FLASH' scripts/run_patch_005_hardware_live.py
grep -q 'Type OAUTH' scripts/run_patch_005_hardware_live.py

if grep -R -n -E 'client_secret[[:space:]]*=[[:space:]]*"[^"]+|access_token[[:space:]]*=[[:space:]]*"[^"]+|refresh_token[[:space:]]*=[[:space:]]*"[^"]+"' \
  components scripts docs --exclude='test_athom_cloud_native.c'; then
  echo "FAIL: credential literal"; exit 1
fi
if grep -R -n -E 'setCapabilityValue|triggerFlow|activateMood|genericApiCall' \
  components/athom_cloud_native/src \
  components/athom_cloud_native/include; then
  echo "FAIL: mutation surface"; exit 1
fi
git diff --check
echo "PASS: Patch 005 executable OAuth offline validation"
