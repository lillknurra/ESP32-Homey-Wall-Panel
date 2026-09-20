#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

printf '%s\n' 'PATCH039_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN'
printf '%s\n' 'PATCH039_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'REFUSED: Patch039 offline foundation does not wire a live Homey factory or read credentials.' >&2
exit 3
