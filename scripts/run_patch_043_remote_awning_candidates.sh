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

printf '%s\n' 'PATCH046_HOMEY_RUNTIME=DIRECT_PINNED_HOMEY_API__READ_ONLY_OAUTH_STORE'
printf '%s\n' 'PATCH047_STORAGE_ADAPTER=ATHOMCLOUDAPI_STORAGEADAPTER_SUBCLASS'
printf '%s\n' 'PATCH050_OAUTH_REFRESH=HTTP_401_ONLY__ONE_VOLATILE_ATTEMPT'
printf '%s\n' 'PATCH050_AUTO_REFRESH_TOKENS=false'
printf '%s\n' 'PATCH050_OAUTH_DISK_WRITE=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH050_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH050_OAUTH_CLIENT_CONFIG=EXTERNAL_PROCESS_ENVIRONMENT_REQUIRED'
if test -z "${ATHOM_API_CLIENT_ID:-}" || test -z "${ATHOM_API_CLIENT_SECRET:-}"; then
  printf '%s\n' 'REFUSED: Patch050 requires ATHOM_API_CLIENT_ID and ATHOM_API_CLIENT_SECRET in the process environment.' >&2
  exit 2
fi

npm --prefix tools/homey-inventory run build
exec node tools/homey-inventory/dist/src/awning-athom-remote-candidates.js "$@"
