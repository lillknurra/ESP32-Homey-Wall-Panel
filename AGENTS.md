# Agent Instructions

Use conservative, evidence-driven, patch-based engineering. Preserve verified behavior and prefer small, traceable, reversible changes.

## Session start and document ownership

Read `prompts/START_PROMPT.md`, then follow the official reading order in `docs/handoff/MASTER_INDEX.md`. `PROJECT_INSTRUCTIONS.md` owns permanent project constraints. `docs/handoff/HANDOFF.md` is the authoritative project-status and resume point; supporting architecture, workflow, history and evidence documents own their respective details.

Before continuing any work, verify the actual Git branch, HEAD, remote tracking state and worktree. Compare them with HANDOFF. Stop if they conflict; never infer Git state from documentation or chat.

## Source, build, runtime and device boundaries

- The Git repository is authoritative for source. Documentation records intent; accepted validation evidence determines only the evidence class it actually proves.
- Keep source, build, runtime, integration, protocol, firmware, hardware, synchronization, packaging and measurement evidence distinct. A build does not prove runtime or hardware behavior.
- Verify the documented ESP-IDF `v6.0.1` toolchain before build or flash. Flashing, serial capture and device operations are separate gated phases and require the applicable current operator authorization and accepted inputs.
- Homey communication is Internet/Athom API only. Local/LAN/PAT fallback and implicit browser login are forbidden. Do not perform Homey operations unless the current task explicitly authorizes the exact operation.

## Git and patch rules

- Use one logical purpose per patch and commit. Audit the baseline, define scope and non-goals, validate the exact local files, inspect the full diff, stage exact paths, and inspect staged names, statistics and diff before committing.
- Do not rewrite published history, merge, push or open a PR unless explicitly authorized for that action. Verify remote state before claiming publication or completion.
- Follow the repository patch workflow and preserve failed evidence. Do not create follow-up bookkeeping patches solely to record a self-finalizing documentation patch's own merge.

## Documentation-only publication

Simple documentation changes may go directly to `main` without a separate Patch,
feature branch or pull request. This includes Scratchpad/checklists, current state,
handoff, roadmap, corrections and COMPLETE/SKIPPED/DEFERRED/PENDING status updates.

- Verify the Git baseline and protect existing work, using an isolated clean
  worktree when needed. Only documentation may change under this exception.
- Review the complete diff and privacy, stage exact paths and create one clear
  documentation commit. Push to `main` requires explicit authorization for the
  current task; use a normal fast-forward push only.
- Never force-push or bypass branch protection. If direct push is rejected, stop.
- The normal controlled development process still applies to functional code,
  security-critical changes, NVS, OTA, Homey control and firmware. Describing a
  future feature in a roadmap does not authorize implementing or operating it.

## Security and privacy

- Never commit secrets, credentials, tokens, Wi-Fi details, raw private device identifiers or device-specific provisioning values. Keep credentials out of command output and shared evidence; sanitize logs and reports before sharing.
- Treat OAuth, settings stores, tokens, Homey state and device provisioning as protected data. Do not write or refresh persistent credentials, start browser login, use local/LAN/PAT fallback, or mutate Homey unless an explicit, bounded authorization and project policy permit it.

## Operator and write gates

- Do not perform external writes, device/NVS/flash/serial operations, Homey mutations, commits, merges or pushes beyond the user's explicit authorization. Keep operator actions separate from local validation and provide exact commands, expected results and evidence requirements when an operator gate is needed.
- Never promote `FAIL`, `INCONCLUSIVE` or `NOT RUN` to `PASS`. Use `PASS` only for the named gate supported by accepted evidence. Use `COMPLETE` only when every required workflow gate has passed and any required remote verification is complete.
- State assumptions, risks, unknowns and validation status. Analyze supplied failure evidence before proposing a correction; distinguish validator, syntax, build, hash and packaging failures from implementation failures.
- When an operator must act, give copy/paste instructions, explicit pass/fail criteria and the exact evidence to return; end each work step with a complete `## Nästa prompt` continuation block.
## Execution efficiency

- Do not re-read files that have already been inspected unless they changed.
- Do not rerun passing tests unless code affecting them changed.
- Prefer targeted searches over repository-wide scans.
- Inspect the minimum source set necessary for the current task.
- Do not investigate historical patches unless directly relevant.
- Run independent read-only checks in parallel where supported.
- Keep terminal output concise.
- Do not repeat previously established evidence.

## ChatGPT ↔ Codex handoff

`CHATGPT_CODEX_HANDOFF_MODE=MANUAL_COPY_PASTE`

### Roles

**ChatGPT**

- Analyzes source code, logs, tests and validation evidence.
- Separates confirmed root causes from hypotheses.
- Evaluates risks and defines technical patch scope together with the operator.
- Prepares complete task prompts for VS Code Codex.
- Recommends a Codex model, reasoning level, approximate token burden and
  conditions for escalating the work.
- Reviews Codex results before recommending the next step.
- Does not treat Codex-generated continuation prompts as automatic instructions.

**VS Code Codex**

- Implements only explicitly scoped changes.
- Writes and runs tests, and builds firmware when required by the authorized
  task.
- Reviews complete and staged Git diffs.
- Creates local commits only within the authorized scope.
- Reports results, evidence, remaining risks, hashes and changed files.
- May suggest a subsequent task but does not start it autonomously.
- Stops at the task's defined authorization boundary.

**Operator**

- Agrees technical direction together with ChatGPT.
- Transfers prompts to Codex and returns Codex results to ChatGPT.
- Executes physical operations.
- Explicitly authorizes flashing, publication, PR creation, merging and other
  protected actions.
- Does not implicitly delegate authorization through a previous prompt.

### Default development cycle

ChatGPT analysis and patch definition → operator agreement → ChatGPT task prompt
for Codex → Codex implementation and tests → operator returns the result →
ChatGPT review → next decision.

This is the default process unless the operator explicitly chooses another
workflow for an individual task.

- ChatGPT provides the complete next task prompt under `## Nästa prompt`.
- The operator manually copies that prompt into Codex.
- Codex performs the task and returns its result in the Codex chat.
- The operator manually copies the Codex result back to ChatGPT.
- `.codex-handoff/NEXT_TASK.md` is `DEPRECATED / DO_NOT_USE_AUTOMATICALLY`.
- `.codex-handoff/LAST_RESULT.md` is `DEPRECATED / DO_NOT_USE_AUTOMATICALLY`.
- Do not automatically read or write those files, require them to be current or
  present, or use `Codex: Run NEXT_TASK` / `run-next.sh` as the normal workflow.
  Read or use either file only when the current user prompt explicitly requests
  it.
- Verify actual Git state before work. Git is the factual source for current
  state; if it conflicts with a handoff document, report the exact discrepancy
  and stop until it is reconciled.
- Stop before an action requiring operator authorization unless the exact,
  bounded authorization is present in the current user prompt. This includes
  firmware flash; serial work beyond explicitly authorized transport; NVS
  read/write; Homey mutation or device/capability command; provisioning;
  erase/reset/reboot/power cycle; destructive Git actions; push, PR or merge.
  Authorization never carries over from a previous task and expires when its
  bounded gate ends.
- Never write raw tokens, credentials, private Homey/device/capability IDs or
  private binding material to tracked files or shared evidence.
- Reuse passing validation only when relevant source identity is unchanged. If a
  task defines sequential fail-closed validation, stop at its first failure.
