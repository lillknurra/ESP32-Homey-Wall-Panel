#!/usr/bin/env bash
set -euo pipefail

EXPECTED_HEAD="fe153af0996bfafb3fc4a5f529e25b0eb4a88412"
EXPECTED_BRANCH="agent/bootstrap-project"

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }
pass() { printf 'PASS: %s\n' "$*"; }

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || fail "not inside a Git repository"
cd "$ROOT"

[[ "$(git branch --show-current)" == "$EXPECTED_BRANCH" ]] || fail "unexpected branch"
[[ "$(git rev-parse HEAD)" == "$EXPECTED_HEAD" ]] || fail "unexpected HEAD; Patch 002 must be validated before commit"
pass "branch and starting HEAD"

expected=(
  docs/handoff/CURRENT_STATE.md
  docs/handoff/HANDOFF.md
  docs/hardware/AUDIO_ARCHITECTURE.md
  docs/hardware/DISPLAY_INTERFACE.md
  docs/hardware/HARDWARE_BASELINE.md
  docs/hardware/HARDWARE_EVIDENCE_DATABASE.md
  docs/hardware/MEMORY_CONFIGURATION.md
  docs/hardware/PIN_MAPPING.md
  docs/hardware/POWER_AND_IO_EXPANDER.md
  docs/hardware/PRIMARY_SOURCE_INDEX.md
  docs/hardware/TOUCH_INTERFACE.md
  docs/hardware/evidence/EVIDENCE_CAPTURE_002.md
  docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md
  docs/history/PATCH_HISTORY.md
  scripts/validate_patch_002.sh
)

for path in "${expected[@]}"; do
  [[ -f "$path" ]] || fail "missing expected file: $path"
done
pass "all expected files exist"

bash -n scripts/validate_patch_002.sh
pass "validator shell syntax"

for token in VERIFIED_PRIMARY DERIVED_PRIMARY EXAMPLE_DEPENDENT UNVERIFIED HARDWARE_VALIDATED; do
  grep -Rqs -- "$token" docs/hardware docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md \
    || fail "missing evidence class token: $token"
done
pass "evidence vocabulary"

grep -q 'a9025815a1e51b81dcdc1936271c5bab528fbfc7ea37cd0a790dcb9a81e7e84f' docs/hardware/PRIMARY_SOURCE_INDEX.md \
  || fail "official demo archive hash missing"
grep -q '83528644cac61518a38ca67f315fc19a97851b5dea616d224fa4281d7d3959ff' docs/hardware/PRIMARY_SOURCE_INDEX.md \
  || fail "preserved package hash missing"
grep -q '010c5fb39ea7f0e996efd1f8e162cc70e569e57f' docs/hardware/PRIMARY_SOURCE_INDEX.md \
  || fail "Waveshare BSP commit missing"
pass "source identities are pinned"

grep -q 'GPIO47' docs/hardware/PIN_MAPPING.md || fail "I2C SDA missing"
grep -q 'GPIO48' docs/hardware/PIN_MAPPING.md || fail "I2C SCL missing"
grep -q 'GPIO46' docs/hardware/PIN_MAPPING.md || fail "HSYNC missing"
grep -q 'EXIO5' docs/hardware/TOUCH_INTERFACE.md || fail "touch reset derivation missing"
grep -q 'EXIO6' docs/hardware/TOUCH_INTERFACE.md || fail "touch interrupt derivation missing"
grep -q 'different RGB porch' docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md \
  || fail "timing conflict not preserved"
pass "critical board facts and conflicts"

hardware_validated_mentions="$(
  grep -RniE '(^|[^A-Z_])HARDWARE_VALIDATED([^A-Z_]|$)' docs/hardware || true
)"

unexpected_hardware_validated_mentions="$(
  printf '%s\n' "$hardware_validated_mentions" \
    | grep -viE \
        'contains no claims|no statement|not[[:space:]`*_]*HARDWARE_VALIDATED|no Patch 002 claim' \
    || true
)"

if [[ -n "$unexpected_hardware_validated_mentions" ]]; then
  printf '%s\n' "$unexpected_hardware_validated_mentions" >&2
  fail "unexpected HARDWARE_VALIDATED factual claim"
fi
pass "no hardware validation claim"

changed=()
while IFS= read -r path; do
  [[ -n "$path" ]] && changed+=("$path")
done < <(
  {
    git diff --name-only HEAD
    git ls-files --others --exclude-standard
  } | LC_ALL=C sort -u
)

expected_sorted=()
while IFS= read -r path; do
  [[ -n "$path" ]] && expected_sorted+=("$path")
done < <(
  printf '%s\n' "${expected[@]}" | LC_ALL=C sort -u
)

if ! diff -u <(printf '%s\n' "${expected_sorted[@]}") <(printf '%s\n' "${changed[@]}"); then
  fail "changed path set differs from Patch 002 scope"
fi
pass "exact changed-path scope"

for path in "${changed[@]}"; do
  [[ "$path" != main/* ]] || fail "forbidden main/ change: $path"
  [[ "$path" != "sdkconfig.defaults" ]] || fail "forbidden sdkconfig.defaults change"
done
pass "no firmware or sdkconfig.defaults changes"

printf '\nPatch 002 static validation: PASS\n'
printf 'Evidence proved: documentation structure and exact patch scope only.\n'
printf 'Evidence not proved: build, runtime, integration, protocol, or hardware.\n'
