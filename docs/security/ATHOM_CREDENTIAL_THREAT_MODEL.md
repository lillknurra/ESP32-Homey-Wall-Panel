# Athom credential threat model

## Protected assets

- OAuth client secret;
- access token;
- refresh token;
- selected Homey ID;
- authorization code and OAuth state.

## Physical threats

Flash extraction, debug-port access, firmware replacement and core-dump leakage
can expose credentials. Production deployment therefore requires encrypted NVS,
flash encryption, secure boot, disabled or protected debug access, and redacted
crash diagnostics. These controls reduce risk but cannot turn a physically
extractable embedded OAuth client into a confidential server-side client.

## Network threats

TLS certificate validation is mandatory. OAuth state is random, single-use and
expires after ten minutes. Authorization codes are never persisted. Refresh
failure does not fall back to anonymous or broad access.

## API threats

The native client exposes typed inventory operations only. Generic paths,
generic HTTP forwarding and mutation methods are prohibited. The mutation
allowlist is empty in Patch 005.

## Logging

Status names may be logged. Tokens, client secret, Homey ID, authorization code,
raw HTTP bodies and private provisioning values must never be logged.
