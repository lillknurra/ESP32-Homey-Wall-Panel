# Development Workflow

1. Audit branch, local HEAD, remote HEAD, worktree, and documented baseline.
2. Define one patch purpose, non-goals, expected files, validation, rollback, and commit message.
3. Implement locally using deterministic scripts when practical.
4. Run static checks and scope-appropriate validation.
5. Inspect the exact diff.
6. Stage exact files.
7. Commit and push only after accepted validation.
8. Verify remote commit and open a draft PR.
9. Lock durable documentation from accepted evidence. The intended stable
   post-merge state must identify `main` as the stable branch and must not
   invent or authorize a subsequent patch.
10. After merge, verify the merged `main` ref. A bounded finalization patch may
    record the preceding merge, but it must be self-finalizing and must not
    require another state-lock patch solely to record its own merge.
11. Start later work only after a separate patch scope decision.

## Proportional controls

Use the smallest control sequence that proves the required invariants. Prefer one
repeatable validator or wrapper over long manual command blocks, but do not remove
scope checks, full diff review, evidence boundaries or explicit approval gates.
Do not rerun equivalent expensive checks without a new reason or changed input.

## Preferred package execution

For operator-run packages, prefer one ZIP containing one main entrypoint named
`apply_validate_and_collect.py`.

- Unpack into an isolated `mktemp` directory.
- Apply executable permissions inside that directory.
- Run one main script that applies, validates and collects evidence.
- Preserve a clear process exit status.
- Clean the temporary directory after success or failure.
- Normally leave one timestamped result ZIP under `~/Downloads/` and no unpacked
  package or loose temporary output.
- A validation-only package must leave the repository unchanged.
- An application package may modify only its exact approved and hash-protected scope.
- Stop before commit, push or pull request unless each action is separately approved.

Local operator output uses `~/Downloads/`. CI or another headless environment may
use an explicitly configured output directory outside the repository. Do not
implicitly create an evidence directory inside the repository.

## Git and publication

Use normal local Git for multi-file changes. Verify exact staging, staged names,
diff statistics, staged diff and `git diff --cached --check` before commit. Use
GitHub connectors for repository state and PR administration, not as a substitute
for a normal local commit and push when local Git is available.

## Flash separation

Flash is a separate explicitly approved phase after every applicable local check
passes. Applying a source or documentation package must never automatically flash
a device. A hardware runner may flash only after its exact firmware input and
local validation evidence have been accepted.
