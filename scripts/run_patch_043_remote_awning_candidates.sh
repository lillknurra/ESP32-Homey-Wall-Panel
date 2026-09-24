#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX
export HOMEY_SKIP_STARTUP_NOTIFIERS=1

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if test -n "${HOMEY_PAT:-}"; then
  printf '%s\n' 'REFUSED: Patch043 forbids HOMEY_PAT/token mode; use Athom OAuth only.' >&2
  exit 2
fi

printf '%s\n' 'PATCH043_TRANSPORT=ATHOM_INTERNET_ONLY'
printf '%s\n' 'PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN'
printf '%s\n' 'PATCH043_FLOW_READ=NOT_RUN'
printf '%s\n' 'PATCH043_ADVANCED_FLOW_READ=NOT_RUN'
printf '%s\n' 'PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PATCH043_FIRMWARE_CHANGE=NONE'

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
if test "$NODE_MAJOR" -lt 24; then
  printf 'REFUSED: Node 24 or newer is required; observed major=%s\n' "$NODE_MAJOR" >&2
  exit 2
fi

printf '%s\n' 'PATCH045_HOMEY_CLI_RESOLUTION=PRODUCT_RUNTIME_MULTI_ROOT_FAIL_CLOSED'

npm --prefix tools/homey-inventory run build
exec node tools/homey-inventory/dist/src/awning-athom-remote-candidates.js "$@"
