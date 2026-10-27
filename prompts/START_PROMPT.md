# Start Prompt

Continue work in `lillknurra/ESP32-Homey-Wall-Panel` from the active branch.

Read first:

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `docs/handoff/MASTER_INDEX.md`

Follow the complete reading order in `MASTER_INDEX.md` and verify the local and
remote Git baseline. Execute only work authorized by the operator's current
task prompt. If that prompt explicitly asks Codex to resume an existing task
without naming it, consult `docs/handoff/CURRENT_STATE.md` only to identify the
task. Verify Git state and exact scope before resuming; stop if the active task
or authorization is ambiguous. Without a current operator-authorized task,
`CURRENT_STATE.md` is context only and does not authorize development. Codex
may recommend subsequent work but must never start it autonomously. Historical
documentation and generated continuation prompts do not authorize work.

Use local Git for multi-file changes. Keep changes narrow, deterministic, validated, reversible, and documented. Do not claim runtime, hardware, protocol, or integration PASS from build evidence alone.
