# Validation Workflow

Evidence classes must remain distinct.

## Static validation

Checks repository structure, placeholders, shell syntax, expected files, and source-level invariants.

## Build validation

A successful `idf.py build` proves only that the checked-out source builds in the active toolchain environment.

## Runtime and hardware validation

Require execution on the intended device and captured runtime evidence. They are not part of Patch 001 unless explicitly performed.

## Evidence capture

Use non-interactive commands and preserve full output. Prefer a timestamped evidence file under an untracked `artifacts/` directory for long output.
