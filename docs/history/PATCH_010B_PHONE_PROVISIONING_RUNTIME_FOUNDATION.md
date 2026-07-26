# Patch 010B - Phone Provisioning Runtime Foundation

## Purpose
Implement a hardware-runnable local phone provisioning flow using a synthetic OAuth provider, explicit Homey selection, atomic NVS publication, reboot restore and Homey-only wipe.

## Base
`main` at `a8c460873432f930d7632de298d2e87e9d0848f1`.

## Boundaries
No real Athom credentials, login, endpoints, Homey traffic or mutation. Normal NVS is used; encrypted NVS, Secure Boot, flash encryption and eFuse remain NOT RUN.

## Validation
Host tests, validator, `git diff --check`, ESP-IDF v6.0.1 build, followed by separate flash, serial, phone, persistence and wipe verification before merge.
