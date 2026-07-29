#!/bin/sh

validate_patch_011x() {
  BASE="b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca"
  EXPECTED_BRANCH="patch-011x-post-merge-baseline-athom-architecture"

  CURRENT_BRANCH="$(git branch --show-current)"
  if [ "$CURRENT_BRANCH" != "$EXPECTED_BRANCH" ]; then
    echo "FAIL: branch är $CURRENT_BRANCH, förväntade $EXPECTED_BRANCH"
    return 1
  fi

  EXPECTED="$(mktemp)"
  ACTUAL="$(mktemp)"

  printf '%s\n' \
    "docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md" \
    "docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md" \
    "docs/architecture/HOMEY_INVENTORY_CONTRACT.md" \
    "docs/handoff/CURRENT_STATE.md" \
    "docs/handoff/HANDOFF.md" \
    "docs/handoff/MASTER_INDEX.md" \
    "docs/history/PATCH_011X_POST_MERGE_BASELINE_AND_ATHOM_ARCHITECTURE_RECONCILIATION.md" \
    "docs/history/PATCH_HISTORY.md" \
    "scripts/validate_patch_011x.sh" \
    | sort > "$EXPECTED"

  {
    git diff --name-only "$BASE"
    git ls-files --others --exclude-standard
  } | sort -u > "$ACTUAL"

  if ! cmp -s "$EXPECTED" "$ACTUAL"; then
    echo "FAIL: scope avviker"
    diff -u "$EXPECTED" "$ACTUAL"
    rm -f "$EXPECTED" "$ACTUAL"
    return 1
  fi

  rm -f "$EXPECTED" "$ACTUAL"

  grep -q 'Patch 011X - Post-Merge Baseline and Athom Architecture Reconciliation' docs/handoff/CURRENT_STATE.md || return 1
  grep -q 'COMPLETE / MERGED' docs/history/PATCH_HISTORY.md || return 1
  grep -q 'PR #15' docs/handoff/CURRENT_STATE.md || return 1
  grep -q 'b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca' docs/handoff/CURRENT_STATE.md || return 1
  grep -q 'Patch 012 is only a possible next functional candidate and is not active' docs/handoff/CURRENT_STATE.md || return 1
  grep -q 'no Patch 011Y' docs/history/PATCH_011X_POST_MERGE_BASELINE_AND_ATHOM_ARCHITECTURE_RECONCILIATION.md || return 1

  if grep -R -n -E 'Active patch: Patch 011|DOCUMENTATION FINALIZATION IN PROGRESS / NOT COMMITTED' docs/handoff; then
    echo "FAIL: stale aktuell Patch 011-status finns kvar"
    return 1
  fi

  if grep -R -n -E 'The current offline implementation includes|Athom support for redirect formats.*remains open|Real Athom OAuth, live Homey traffic and real protocol compatibility remain NOT RUN or NOT VERIFIED' \
    docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md \
    docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md; then
    echo "FAIL: stale aktuell Athom-arkitekturstatus finns kvar"
    return 1
  fi

  if grep -R -n -E '(client_secret|refresh_token|access_token|Authorization: Bearer)[[:space:]]*[:=][[:space:]]*[^`[:space:]]+' docs/handoff docs/architecture docs/history/PATCH_011X_POST_MERGE_BASELINE_AND_ATHOM_ARCHITECTURE_RECONCILIATION.md; then
    echo "FAIL: möjlig secret eller credential hittades"
    return 1
  fi

  git diff --check || return 1
  echo "PATCH_011X_VALIDATION PASS"
  return 0
}

validate_patch_011x
