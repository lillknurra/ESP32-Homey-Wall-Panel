# Patch 005 - Athom Cloud Transport and Provisioning

## Purpose

Add the official endpoint registry, ESP-IDF HTTPS/TLS transport boundary,
bounded form and response handling, and an explicitly disabled-by-default
provisioning portal.

## Evidence class

This phase is offline source, host-test and ESP-IDF build evidence. Real OAuth,
Homey session establishment and inventory remain NOT RUN until a separately
authorized local hardware run.

## Safety decisions

- only official Athom and Homey remote URL families are accepted;
- response bodies are capped at 64 KiB;
- mutation paths are rejected;
- provisioning live submission starts disabled;
- no credentials appear in source, tests, evidence or logs;
- unsupported OAuth grants are not implemented.
