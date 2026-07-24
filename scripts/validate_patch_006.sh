#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

EXPECTED_BRANCH="patch-006-controlled-private-credential-preflight"
EXPECTED_BASE="2e7454a7fffac63c509e1c7751c54b33206f6052"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

require_literal() {
  local pattern="$1"
  local path="$2"
  grep -Fq "$pattern" "$path" || fail "missing required literal in $path: $pattern"
}

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail branch
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail local-main
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail origin-main
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail base-ancestor

EXPECTED="$(mktemp)"
ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL"' EXIT

printf '%s\n'   docs/handoff/CURRENT_STATE.md   docs/handoff/HANDOFF.md   docs/handoff/MASTER_INDEX.md   docs/history/PATCH_006_CONTROLLED_PRIVATE_CREDENTIAL_PREFLIGHT_VALIDATION.md   docs/history/PATCH_HISTORY.md   scripts/validate_patch_006.sh | sort > "$EXPECTED"

{
  git diff --name-only
  git ls-files --others --exclude-standard
} | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"

diff -u "$EXPECTED" "$ACTUAL" || fail exact-six-file-scope

test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "6" || fail file-count

bash -n scripts/validate_patch_006.sh
git diff --check

# Production and historical validator paths must remain untouched.
if grep -E '^(components/|main/|tools/homey-inventory/(src|test)/|sdkconfig.defaults$|partitions.csv$|scripts/validate_patch_005)' "$ACTUAL"; then
  fail forbidden-production-or-historical-validator-change
fi

# Existing Keychain existence check must not request credential value output.
CHECK_BODY="$(sed -n '/export async function checkCredentialEntryExists/,/^}/p' tools/homey-inventory/src/credential-provider.ts)"
printf '%s\n' "$CHECK_BODY" | grep -Fq 'find-generic-password' || fail missing-existence-check
if printf "%s\n" "$CHECK_BODY" | grep -Eq "(^|[^[:alnum:]_])-w([^[:alnum:]_]|$)"; then
  fail credential-value-output-option
fi

# Sanitized evidence must keep all forbidden actions as literal false.
EVIDENCE="tools/homey-inventory/src/preflight-evidence.ts"
require_literal 'credential_value_read: false' "$EVIDENCE"
require_literal 'homey_client_constructed: false' "$EVIDENCE"
require_literal 'discovery_attempted: false' "$EVIDENCE"
require_literal 'authentication_attempted: false' "$EVIDENCE"
require_literal 'network_access_attempted: false' "$EVIDENCE"

# Patch files may describe field names and conventions, but may not commit selector
# assignments, Keychain mutation commands, credential material or private config.
PATCH_FILES=(
  docs/handoff/CURRENT_STATE.md
  docs/handoff/HANDOFF.md
  docs/handoff/MASTER_INDEX.md
  docs/history/PATCH_006_CONTROLLED_PRIVATE_CREDENTIAL_PREFLIGHT_VALIDATION.md
  docs/history/PATCH_HISTORY.md
  scripts/validate_patch_006.sh
)

if grep -nE '"keychain_(service|account)"[[:space:]]*:[[:space:]]*"[^"<]+' "${PATCH_FILES[@]}"; then
  fail committed-keychain-selector
fi
if grep -nE 'security[[:space:]]+(add|delete)-generic-password|find-generic-password[[:space:]].*-w' "${PATCH_FILES[@]}"; then
  fail forbidden-keychain-operation
fi
if grep -nE '(access_token|refresh_token|client_secret|authorization_code)[[:space:]]*[:=][[:space:]]*[^[:space:]`"]+' "${PATCH_FILES[@]}"; then
  fail credential-material
fi

# Offline build/tests are permitted only from already-present dependencies.
test -d tools/homey-inventory/node_modules || fail 'node_modules absent; network install is forbidden'
npm --offline --prefix tools/homey-inventory run build
npm --offline --prefix tools/homey-inventory test

git diff --check
printf 'PASS: Patch 006 offline evidence and documentation validation\n'
