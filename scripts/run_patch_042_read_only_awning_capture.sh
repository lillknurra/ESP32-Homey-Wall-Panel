#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

printf '%s\n' 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PATCH042_FLOW_EXECUTION=NOT_RUN'
printf '%s\n' 'PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN'
printf '%s\n' 'PATCH042_FIRMWARE_CHANGE=NONE'

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
if [ "$NODE_MAJOR" -lt 24 ]; then
  printf 'PATCH042_FAIL=Node 24 or newer is required; observed major=%s\n' "$NODE_MAJOR" >&2
  exit 2
fi

npm --prefix tools/homey-inventory run build
exec node tools/homey-inventory/dist/src/awning-live-capture.js "$@"
