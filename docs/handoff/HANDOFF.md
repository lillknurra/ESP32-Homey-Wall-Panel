# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Stable baseline

- stable branch: `main`;
- stable commit: `b36d77b2e655be546d93651ac4a06412355dfab2`;
- local `main` and `origin/main`: identical at the verified post-merge gate;
- working tree: clean at the verified post-merge gate;
- Patch 012: `COMPLETE / MERGED` through PR #17;
- Package 3A: `COMPLETE`;
- Package 3B: `NOT_STARTED`.

## Active work

No development patch is active.
No development branch is active.
The next patch is `UNDECIDED` pending a separate requirements and scope
decision.

Do not resume Patch 012. Do not create another Patch 012 state-lock, merge-SHA
or finalization patch solely to record PR #17 or
`b36d77b2e655be546d93651ac4a06412355dfab2`.

## Package 3A accepted evidence

- production finalizer v3.4: PASS;
- working-tree files at the final feature gate: exactly 20;
- full Patch 012 union: exactly 23;
- locked feature hashes: 22/22 PASS;
- original Waveshare managed-component hashes restored;
- host tests: PASS;
- ESP-IDF v6.0.1 production build and `idf.py size`: PASS;
- normal flash and verification: PASS;
- erase-flash: NOT RUN;
- passive `tools/serial_monitor.py` session without `--reset`: PASS;
- physical dimming, visual-off and normal touch: PASS;
- known product defects: none.

The accepted runtime sequence was 80 to 30 to 0 to 80. It recorded ACTIVE to
DIMMED after about 10 seconds, DIMMED to OFF after about 60 seconds and wake
back to normal brightness. Every observed brightness request returned `ESP_OK`
and display refresh resumed after wake. No panic, assert, watchdog, brownout,
heap, stack or unexpected reboot marker was observed.

## Evidence limitation

The v5.3 interactive runner's `normal_touch_ok=FAIL` is not product evidence.
The abnormal behavior occurred while the runner used `--reset` on the native
USB serial port. A later passive monitor session without `--reset` verified
normal panel and touch behavior. Classify v5.3 as test-harness interference,
not `FAIL_DISPLAY`.

The planned formal two-cycle matrix at each dim level 10, 30 and 50 was not
completed as a recorded matrix. Keep this as
`FORMAL_SIX_CYCLE_MATRIX=INCOMPLETE_NON_BLOCKING`; do not claim that all six
cycles were formally completed.

No additional automated interactive question runners are required.

## Required next step

- verify the current local `main` baseline before new work;
- keep Package 3B as `NOT_STARTED`;
- make a separate requirements and scope decision;
- name and create the next feature branch only after that decision;
- do not perform code, build, size, flash or hardware work as part of this
  handoff correction.
