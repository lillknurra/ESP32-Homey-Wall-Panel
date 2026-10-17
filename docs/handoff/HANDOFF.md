# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md owns the detailed current roadmap;
CURRENT_STATE.md separates source, test/build and operator/runtime evidence.

## Verified state – 2026-10-17

- Stable branch: main, repository lillknurra/ESP32-Homey-Wall-Panel.
- Patch086 COMPLETE / MERGED, PR #97. Verified functional merge
  `719fa856bcc2b99f437511895722240dd04ae278`, tree
  `17d2aa878cf147ea67dfc2eed3f0e68c95e3614d`.
  Documentation-only commits may advance main; verify actual remote and local
  Git state rather than treating this historical anchor as a frozen HEAD.
- Installed Patch086 BIN SHA-256:
  `1598ecf17f8748e4046392688ef2fa232d150ff80414c28c97a9f8b4f0b115c4`.
- Operator confirms correct Solskydd/Balkongmarkis status, normal Homey
  connection and no full-screen flicker in observed use. Flicker root cause
  remains INCONCLUSIVE; no RGB fix is claimed.
- Patch083 recovery, Patch084 UI feedback and Patch078/079E Wi-Fi protections
  remain. Patch085 control implementation is merged, but awning_2/3 write
  flags remain 0/0; awning_1 remains disabled. No movement is verified.

## Next functional work

1. Implement read-only Advanced status/version/time/diagnostics from existing
   runtime sources, without new Homey operations or persistence side effects.
2. Design secure HTTPS OTA and a separate controlled one-time migration:
   hardware/version/authenticity checks, two app slots, otadata, boot validation
   and automatic rollback, with protected configuration and explicit approval.
   Current factory-only layout/bootloader do not supply these prerequisites.
3. Continue separately gated awning_2/3 control verification; active private
   binding, capability semantics and authoritative physical confirmation remain
   prerequisites. Never enable writes merely because readiness gates pass.

Then separate awning_1 UPP/STOPP/NED; retain Wi-Fi physical validation,
backup/restore, stability/memory and future Verisure. See SCRATCHPAD for complete
status and per-function scope; do not implement any next task automatically.

## Evidence limits

- Recurrent physical flicker debugging is DEFERRED while the latest panel is
  flicker-free. Long-term/startup stability and target memory margin are PENDING.
- NVS_BACKUP=NOT_VERIFIED; ordinary app-only writing does not guarantee absence
  of firmware-triggered persistence. OTA migration requires a separate layout,
  bootloader, security/encryption and restore-readiness assessment.
- Broader host tests remain PARTIAL for the two FAIL_PREEXISTING suites:
  Favorites assertion at test_panel_homey_favorites.c:144 and Patch075's obsolete
  hamburger-helper assertion. Do not promote them without new evidence.
- Actual incomplete-response recovery, Wi-Fi credential/cancel/event races,
  clock DST and full settings persistence/error testing retain target limits.
  Earlier READY/snapshot diagnostics and latest visual confirmation prove
  different evidence classes; correct displayed values alone are not a new
  instrumented snapshot or movement confirmation.

## Resume protocol

1. Read prompts/START_PROMPT.md and the MASTER_INDEX.md reading order.
2. Verify branch, HEAD, tracking state, remote and worktrees directly. Preserve
   unrelated dirty worktrees; use an isolated clean copy when needed.
3. Compare with this handoff; stop on unexpected Git changes. Consult the
   dated Scratchpad summary before historical NEXT_ACTION/PENDING entries.
4. Simple docs may use direct main only with explicit current-task commit/push
   authorization and the AGENTS.md safeguards; no force or protection bypass.
5. No Homey/device/NVS/serial/reset/flash or motion authorization is conveyed by
   this handoff. Request a separately bounded task before functional work.
