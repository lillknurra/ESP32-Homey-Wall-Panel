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

## Security and privacy

- Never commit secrets, credentials, tokens, Wi-Fi details, raw private device identifiers or device-specific provisioning values. Keep credentials out of command output and shared evidence; sanitize logs and reports before sharing.
- Treat OAuth, settings stores, tokens, Homey state and device provisioning as protected data. Do not write or refresh persistent credentials, start browser login, use local/LAN/PAT fallback, or mutate Homey unless an explicit, bounded authorization and project policy permit it.

## Operator and write gates

- Do not perform external writes, device/NVS/flash/serial operations, Homey mutations, commits, merges or pushes beyond the user's explicit authorization. Keep operator actions separate from local validation and provide exact commands, expected results and evidence requirements when an operator gate is needed.
- Never promote `FAIL`, `INCONCLUSIVE` or `NOT RUN` to `PASS`. Use `PASS` only for the named gate supported by accepted evidence. Use `COMPLETE` only when every required workflow gate has passed and any required remote verification is complete.
- State assumptions, risks, unknowns and validation status. Analyze supplied failure evidence before proposing a correction; distinguish validator, syntax, build, hash and packaging failures from implementation failures.
- When an operator must act, give copy/paste instructions, explicit pass/fail criteria and the exact evidence to return; end each work step with a complete `## Nästa prompt` continuation block.
