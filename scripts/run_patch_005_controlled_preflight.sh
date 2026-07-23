#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

if test "${PATCH_005_PREFLIGHT_AUTHORIZED:-}" != "YES"; then
  printf 'REFUSED: set PATCH_005_PREFLIGHT_AUTHORIZED=YES only for controlled preflight.\n' >&2
  exit 2
fi
if test -z "${PATCH_005_PRIVATE_CONFIG:-}"; then
  printf 'REFUSED: PATCH_005_PRIVATE_CONFIG must name an absolute private config path.\n' >&2
  exit 2
fi
REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT/tools/homey-inventory"
npm run build
node dist/src/preflight-runner.js --preflight-only "$PATCH_005_PRIVATE_CONFIG" "$REPO_ROOT"
