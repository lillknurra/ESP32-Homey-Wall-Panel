#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

usage() {
  printf '%s\n' \
    'Usage:' \
    '  PATCH042_PRIVATE_CONFIG=/absolute/config.json scripts/run_patch_042_live_read_only_awning_capture.sh inspect' \
    '  PATCH042_PRIVATE_CONFIG=/absolute/config.json PATCH042_MAPPING=/absolute/mapping.json PATCH042_AWNING_1_ALIAS=device_x PATCH042_AWNING_2_ALIAS=device_y PATCH042_AWNING_3_ALIAS=device_z scripts/run_patch_042_live_read_only_awning_capture.sh map' \
    '  PATCH042_PRIVATE_CONFIG=/absolute/config.json PATCH042_MAPPING=/absolute/mapping.json PATCH042_OUTPUT_DIR=/absolute/output scripts/run_patch_042_live_read_only_awning_capture.sh capture'
}

MODE="${1:-}"
case "$MODE" in
  inspect|map|capture) ;;
  *) usage; fail 'mode must be inspect, map, or capture' ;;
esac

: "${PATCH042_PRIVATE_CONFIG:?PATCH042_PRIVATE_CONFIG must be an absolute private config path}"
case "$PATCH042_PRIVATE_CONFIG" in /*) ;; *) fail 'PATCH042_PRIVATE_CONFIG must be absolute' ;; esac

if [ "$MODE" != "inspect" ]; then
  : "${PATCH042_MAPPING:?PATCH042_MAPPING must be an absolute private mapping path}"
  case "$PATCH042_MAPPING" in /*) ;; *) fail 'PATCH042_MAPPING must be absolute' ;; esac
fi

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
[ "$NODE_MAJOR" -ge 24 ] || fail 'Node.js >=24 is required by tools/homey-inventory'

npm --prefix tools/homey-inventory run build >/dev/null
CLI="$ROOT/tools/homey-inventory/dist/src/awning-live-capture-cli.js"
[ -f "$CLI" ] || fail 'Patch042 compiled CLI is missing'

printf '%s\n' 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PATCH042_FLOW_EXECUTION=NOT_RUN'
printf '%s\n' 'PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN'
printf 'PATCH042_MODE=%s\n' "$MODE"

case "$MODE" in
  inspect)
    exec node "$CLI" inspect --private-config "$PATCH042_PRIVATE_CONFIG"
    ;;
  map)
    : "${PATCH042_AWNING_1_ALIAS:?PATCH042_AWNING_1_ALIAS is required}"
    : "${PATCH042_AWNING_2_ALIAS:?PATCH042_AWNING_2_ALIAS is required}"
    : "${PATCH042_AWNING_3_ALIAS:?PATCH042_AWNING_3_ALIAS is required}"
    GENERATION="${PATCH042_MAPPING_GENERATION:-1}"
    exec node "$CLI" map \
      --private-config "$PATCH042_PRIVATE_CONFIG" \
      --mapping "$PATCH042_MAPPING" \
      --generation "$GENERATION" \
      --awning-1 "$PATCH042_AWNING_1_ALIAS" \
      --awning-2 "$PATCH042_AWNING_2_ALIAS" \
      --awning-3 "$PATCH042_AWNING_3_ALIAS"
    ;;
  capture)
    : "${PATCH042_OUTPUT_DIR:?PATCH042_OUTPUT_DIR must be an absolute private output directory}"
    case "$PATCH042_OUTPUT_DIR" in /*) ;; *) fail 'PATCH042_OUTPUT_DIR must be absolute' ;; esac
    exec node "$CLI" capture \
      --private-config "$PATCH042_PRIVATE_CONFIG" \
      --mapping "$PATCH042_MAPPING" \
      --output "$PATCH042_OUTPUT_DIR"
    ;;
esac
