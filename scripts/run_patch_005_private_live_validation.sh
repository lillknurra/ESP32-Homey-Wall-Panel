#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

if test "${PATCH_005_LIVE_AUTHORIZED:-}" != "YES"; then
  printf 'REFUSED: set PATCH_005_LIVE_AUTHORIZED=YES only during the separately authorized live phase.
' >&2
  exit 2
fi
if test -z "${PATCH_005_PRIVATE_CONFIG:-}"; then
  printf 'REFUSED: PATCH_005_PRIVATE_CONFIG must name an absolute private config path.
' >&2
  exit 2
fi
printf 'Live runner scaffold is present, but live connection construction is still intentionally disabled.
' >&2
exit 3
