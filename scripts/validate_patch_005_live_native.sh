#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"; cd "$ROOT"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/p5-live.XXXXXX")"; trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_live_transport.c \
  -o "$TMP/test_live_transport"
"$TMP/test_live_transport"
! grep -R -n 'not_ready_' components/athom_cloud_native/src/athom_http_esp.c
! grep -R -n 'ATHOM_ERR_UNSUPPORTED' components/athom_cloud_native/src/athom_http_esp.c
awk '/request\(|execute\(/ {c++} END {exit !(c>2)}' components/athom_cloud_native/src/athom_http_esp.c
awk '/basic_header\(/ {c++} END {exit !(c>1)}' components/athom_cloud_native/src/athom_http_esp.c
# Patch 005H.1 uses secure local bootstrap as the active entrypoint.
# Athom runtime and OAuth portal remain compiled and host-tested, but locked.
grep -q 'secure_bootstrap_start' main/main.c
grep -q 'oauth_locked=true' components/secure_bootstrap/secure_bootstrap_esp.c

! grep -q 'athom_runtime_start' main/main.c
! grep -q 'athom_provisioning_portal_start' main/main.c
grep -q '/oauth/callback' components/athom_cloud_native/src/athom_provisioning_portal.c
grep -q '/homeys/select' components/athom_cloud_native/src/athom_provisioning_portal.c
grep -q 'start_new_session=True' scripts/run_patch_005_hardware_live.py
! grep -q 'timeout ' scripts/run_patch_005_hardware_live.py
python3 - <<'PY'
from pathlib import Path
compile(Path('scripts/run_patch_005_hardware_live.py').read_text(), 'runner', 'exec')
PY
git diff --check
echo 'PASS: Patch 005 live native implementation validation'
