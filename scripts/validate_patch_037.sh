#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec python3 "$ROOT/components/secure_bootstrap/test_host/run_homey_light_toggle_dispatch_tests.py" --validate-source
