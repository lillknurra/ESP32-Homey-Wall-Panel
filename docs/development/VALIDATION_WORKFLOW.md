# Validation Workflow

Evidence classes must remain distinct: documentation, packaging, static source,
host test, build, runtime, integration, protocol, firmware, hardware,
synchronization and measurement.

## Static validation

Check repository structure, placeholders, syntax, expected files, exact scope and
source-level invariants. Static evidence does not prove runtime behavior.

## Actual source representation

Write validator rules against the representation that actually exists in the
source file. In C and C++, adjacent C string literals remain separate raw source
fragments even though the compiler concatenates them. Do not require a rendered
runtime sentence to occur as one contiguous source string when it does not.
Account for escapes, macros, function calls and generated representations.

Prefer, where appropriate:

1. parser or syntax validation;
2. compilation;
3. host tests;
4. structural function, macro and call checks;
5. ordering checks;
6. several stable source markers;
7. exact raw-text matching only when the raw text itself is the requirement.

Avoid brittle long exact-text matching when a more structural check proves the
same invariant more reliably.

## Package integrity and idempotence

- Lock the exact file scope.
- Record and verify an exact known BEFORE state and exact expected AFTER state.
- An applicator may accept the exact BEFORE state or the exact already-applied
  AFTER state. Any unknown hash or representation must stop without further change.
- Verify the written file after replacement and keep repeat execution idempotent.
- Verify that payload, wrapper, validator rules and expected ZIP structure agree.
- Reject temporary files, evidence archives and sensitive values inside patch scope.

A deterministic transform may use the repository Git blob hash as its exact
BEFORE identity and must record the resulting AFTER hash in validation evidence.

## Build validation

A successful `idf.py build` proves only that the checked-out source builds in the
verified active toolchain environment. For this project, verify ESP-IDF `v6.0.1`
from `~/GitHub/esp-idf-v6.0.1` when build evidence is in scope.

## Full local gate before flash

Before proposing flash, require PASS for every applicable local class, including
package consistency, known file state, idempotence, syntax, static validator,
host tests, full ESP-IDF build, `git diff --check`, exact scope and secrets review.
Flash remains a separate explicitly approved action.

## Runtime, serial and hardware evidence

Runtime and hardware PASS require execution on the intended device and captured
evidence. A serial logger should wait for a matching USB serial port, use the
documented baudrate, close the port and process correctly, and leave no background
process. A short capture normally stops automatically after 15 seconds; a longer
explicit timeout is allowed when the test requires it. Do not depend on `Ctrl+]`.

Sanitize credentials, SSIDs, BSSIDs, private addresses, panel codes, tokens and
other device-specific provisioning values before sharing evidence. Clearly label
facts reconstructed automatically from artifacts separately from operator-observed
results.

## Evidence capture

Use bounded non-interactive commands and preserve full output. Local operator
runs place generated evidence, logs, reports and normally one timestamped result
ZIP under `~/Downloads/`. CI or headless execution may use an explicitly configured
output directory outside the repository. Do not implicitly create an untracked
`artifacts/` directory inside the repository.
