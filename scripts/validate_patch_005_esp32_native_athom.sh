#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/athom-cloud-host.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
  -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_auth.c \
  components/athom_cloud_native/src/athom_homey_client.c \
  components/athom_cloud_native/src/athom_provisioning.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_athom_cloud_native.c \
  -o "$BUILD_DIR/test_athom_cloud_native"

"$BUILD_DIR/test_athom_cloud_native"

grep -R -n -E 'access-secret|refresh-secret|client-secret' \
  components/athom_cloud_native \
  --exclude='test_athom_cloud_native.c' && {
    printf 'FAIL: secret fixture outside host test\n' >&2
    exit 1
  } || true

if grep -R -n -E 'PKCE|device[_ -]?authorization' components/athom_cloud_native/src; then
  printf 'FAIL: unsupported OAuth capability implemented\n' >&2
  exit 1
fi

grep -q 'return false;' components/athom_cloud_native/src/athom_homey_client.c
bash scripts/validate_patch_005.sh
git diff --check

printf 'Patch 005 ESP32-native Athom Cloud offline validation: PASS\n'
