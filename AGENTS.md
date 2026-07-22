# Agent Instructions

This repository uses conservative, evidence-driven, patch-based engineering.
Preserve verified behavior and prefer small, traceable, reversible changes.

## Source of truth

- The Git repository is authoritative.
- Documentation records intent and durable project knowledge.
- Validation evidence determines correctness.
- Chat history is not a substitute for repository state.

## Read first

Read:

1. `prompts/START_PROMPT.md`
2. `docs/handoff/MASTER_INDEX.md`

Then follow the official reading order in `MASTER_INDEX.md`.

## Documentation ownership

- `AGENTS.md` owns permanent AI and repository behavior.
- `PROJECT_INSTRUCTIONS.md` owns permanent project-specific constraints.
- `docs/handoff/MASTER_INDEX.md` owns the official reading order.
- `docs/handoff/CURRENT_STATE.md` is the sole authority for active branch, baseline, patch status, evidence status, and immediate next work.
- `docs/handoff/HANDOFF.md` carries active work context between sessions.
- `docs/history/` records completed and historical work.

## Patch workflow

```text
BASELINE AUDIT -> IMPLEMENT -> VALIDATE -> PUBLISH -> REMOTE VERIFY -> LOCK -> COMPLETE
```

Never promote `FAIL`, `INCONCLUSIVE`, or `NOT RUN` to `PASS`.

## Repository write policy

For normal code, tooling, test, and documentation changes:

1. edit locally;
2. validate exact local files;
3. inspect the complete diff;
4. commit locally;
5. push the commit;
6. verify the remote ref.

Use GitHub connectors primarily for repository state, PR metadata, comments, reviews, and small administrative changes.

## Git safety

- One logical purpose per patch and commit.
- Stage exact paths only.
- Inspect status, staged names, diff statistics, and full staged diff.
- Do not rewrite published history without explicit authorization.
- Do not claim completion before remote verification.

## Validation and communication

- Claims must match the evidence class actually validated.
- State assumptions, risks, unknowns, and validation status explicitly.
- Provide copy/paste commands for user-run validation.
- Preserve failed evidence.
- Provide one exact next action and a complete continuation prompt.
