# Patch 009 - Offline Athom Session Parsing and Homey Selection

## Purpose

Harden synthetic Athom token, Homey-list, delegation and session parsing, and make
Homey selection deterministic and fail-closed without executing real OAuth, discovery,
inventory or mutation traffic.

## Base

- Branch: `main`
- Commit: `5dcd50093a093b70758b6449911867f1d4428ad3`

## Design

- Exactly one bounded top-level JSON value is accepted.
- Security-relevant duplicate keys, malformed nesting, truncation and trailing data fail closed.
- Caller-owned credential and list state changes only after complete parse success.
- Homey IDs are the sole selection authority; duplicate IDs are rejected.
- Zero, one and multiple candidates remain explicit; no automatic persistent selection occurs.
- Stale selections block inventory and are never silently replaced.
- Delegation and session tokens are parsed as bounded JSON strings and temporary buffers are zeroed.
- Session state is replaced only after complete login success and is invalidated before use for a different Homey.
- A 401 may cause one reauthentication attempt only.
- Errors are stable status codes and never include response bodies or secret values.

## Evidence boundary

Documentation, static source, host tests, validators and ESP-IDF build are required.
Runtime, hardware, flash, serial, real OAuth, real Homey traffic, mutations and protocol
compatibility remain NOT RUN or NOT VERIFIED.

## Non-goals

No real credentials, private URLs or Homey IDs; no display, Wi-Fi, Secure Boot, flash
encryption, eFuse, production-key, encrypted-NVS or anti-rollback work; no Patch 008
merge-SHA-only finalization.

## Completion

Completion requires local package PASS, exact diff review and later separate approval for
staging, commit, push and PR.
