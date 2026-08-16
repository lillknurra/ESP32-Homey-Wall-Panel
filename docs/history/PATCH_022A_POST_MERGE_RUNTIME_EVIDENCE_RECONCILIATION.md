# Patch022A - Post-Merge Runtime Evidence Reconciliation

## Status

- Status: `COMPLETE / MERGED / SELF_FINALIZING`;
- Branch: `patch-022a-finalize-patch022-post-merge-runtime-evidence`;
- Base branch: `main`;
- Base commit: `3cb8993df9a67e105cf70213ac9a5510e32d73dd`;
- Patch022 merge: `3cb8993df9a67e105cf70213ac9a5510e32d73dd`;
- Patch022 PR: `#31`.
- Patch022A PR: `#32`;
- Patch022A squash merge:
  `9de603fd872dceba3fa98ada780fec11eb8dfbe5`;

## Purpose

Reconcile durable repository state after Patch022 was squash-merged and its
approved passive UI runtime evidence was collected. This patch does not change
firmware behavior and does not introduce a new functional optimization.

## Accepted External Evidence

- sanitized log:
  `/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_sanitized.log`;
- summary:
  `/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_summary.txt`;
- log SHA256:
  `2a1b43ff68594816af793cd8892a570baca9f3ba58e00537bab70a9b6ed82207`;
- summary SHA256:
  `f47ae63dcd029465d3442b6323ad94e7eaa5392587ffdf017e1da216cb0254d1`;
- runtime acceptance: `PASS` for the observed passive UI path;
- dashboard scroll: `PASS`;
- settings scroll: `PASS`;
- display performance: `PASS`;
- touch-to-refresh correlation: `OBSERVED`;
- privacy: `PASS`;
- runtime safety: `PASS`.

The evidence does not prove a strict A/B performance improvement over the
former `scroll_throw=4` value. The observed refresh peaks remain documented
residual risk and do not authorize an automatic production fix.

## Durable State

- `main` is the stable branch at
  `9de603fd872dceba3fa98ada780fec11eb8dfbe5`;
- Patch022 is `COMPLETE / MERGED`;
- Patch022A is `COMPLETE / MERGED / SELF_FINALIZING`;
- Patch022A's remote branch cleanup is complete;
- Patch023 is the active documentation-only UI render-path scope lock;
- Package 3B remains `NOT_STARTED`;
- Patch013 runtime remains `NOT_RUN`;
- Patch019 cleanup remains outside scope;
- the next functional scope is a separate UI optimization requirements analysis.

## Exact Scope

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md`.

Allowed new files:

- this document;
- `scripts/validate_patch_022a.sh`.

Forbidden:

- all `components/**`;
- firmware or policy changes;
- Patch019 cleanup;
- Package 3B implementation;
- Patch013 runtime;
- build, flash, erase-flash or runtime;
- branch cleanup.

## Validation

- `scripts/validate_patch_022a.sh` must pass;
- `git diff --check` must pass;
- no component, sdkconfig, build, flash or runtime change is permitted.

After Patch022A was merged and the merged `main` ref was verified, no Patch022B
was created solely to record Patch022A's own merge SHA.
