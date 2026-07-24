# Patch 005 - Controlled Live Preflight

## Status

IMPLEMENTED FOR OFFLINE AND MOCK VALIDATION / REAL PREFLIGHT NOT RUN

## Purpose

Validate private configuration placement, restrictive permissions, provider
availability, and credential-entry existence without reading credential material
and without constructing a Homey client or performing network access.

## Safety boundary

The controlled preflight emits only allowlisted booleans, provider type, mode,
and PASS or REFUSED status. It stores no paths, service names, account names,
selectors, addresses, URLs, raw identifiers, responses, or credential values.

The Keychain existence check omits `-w`; it relies only on the command exit code
and discards subprocess output. Environment-provider support remains an explicit
fallback and is not recommended for routine macOS use.

## Evidence boundaries

- Offline build and tests: required
- Mock controlled preflight: required
- Real private config creation: NOT RUN
- Real Keychain access: NOT RUN
- Credential value read: FORBIDDEN
- Homey client construction: FORBIDDEN
- Discovery, authentication, and network access: FORBIDDEN
- Live Homey collection and publication: NOT RUN

## Controlled preflight environment boundary

The environment provider remains an explicit later live fallback, but controlled
preflight refuses it without reading `process.env`. Existence cannot be checked
without value access, so sanitized evidence returns `REFUSED`,
`provider_available: false`, `credential_entry_exists: false`, and keeps
`credential_value_read: false`.

The private-config setup script uses Node JSON serialization and validates the
written JSON before reporting success. Service and account strings are never
interpolated directly into JSON syntax.
