#!/usr/bin/env bash
set -euo pipefail

fail() { printf 'FAIL: %s\n' "$1" >&2; exit 1; }
pass() { printf 'PASS: %s\n' "$1"; }

test -f tools/homey-inventory/package.json || fail "package.json missing"
test -f schemas/homey_inventory.schema.json || fail "inventory schema missing"
test -f docs/history/PATCH_004_HOMEY_DISCOVERY_AND_INVENTORY_FOUNDATION.md || fail "Patch 004 history missing"
pass "required Patch 004 files exist"

grep -q '"homey-api": "3.19.1"' tools/homey-inventory/package.json || fail "homey-api is not exactly pinned"
pass "official homey-api dependency is exactly pinned"

for forbidden in setCapabilityValue triggerFlow startFlow activateMood genericApiCall; do
  if grep -R --line-number --fixed-strings "$forbidden" tools/homey-inventory/src --exclude='readonly-client.ts'; then
    fail "forbidden mutation symbol present outside boundary declaration: $forbidden"
  fi
done
pass "no Homey mutation operation implemented"

grep -R --line-number   --exclude-dir=node_modules   --exclude-dir=dist   --exclude-dir=private   -E '([0-9]{1,3}\.){3}[0-9]{1,3}|([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}|Bearer[[:space:]]+[A-Za-z0-9._-]+'   tools/homey-inventory   schemas   docs/history/PATCH_004_HOMEY_DISCOVERY_AND_INVENTORY_FOUNDATION.md   && fail "secret/private identifier pattern detected"
pass "tracked Patch 004 content is free of detected private network and token values"

test ! -e tools/homey-inventory/private || fail "private directory must not be tracked"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

(
  cd tools/homey-inventory
  npm_config_cache="$tmp_dir/npm-cache" npm install --ignore-scripts
  npm_config_cache="$tmp_dir/npm-cache" npm test
)
pass "TypeScript build and synthetic unit tests"
node tools/homey-inventory/dist/src/cli.js   --synthetic   --connection auto   --output "$tmp_dir/output"   --alias-registry "$tmp_dir/aliases.json"   --timeout 1000

test -f "$tmp_dir/output/homey_inventory.json" || fail "synthetic JSON output missing"
test -f "$tmp_dir/output/homey_inventory.md" || fail "synthetic Markdown output missing"

for raw in raw-zone-1 raw-zone-2 raw-device-1 raw-flow-1 raw-advanced-flow-1 raw-mood-1; do
  ! grep -R --fixed-strings "$raw" "$tmp_dir/output" || fail "raw ID leaked: $raw"
done
pass "synthetic candidate publication is sanitized"

git diff --check
pass "git diff --check"

printf 'PASS: Patch 004 static, build, unit, synthetic, and privacy validation complete\n'
