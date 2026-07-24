# Patch 005 - Live native execution candidate

This phase replaces the active `not_ready_*` transport functions with concrete,
bounded OAuth token exchange, refresh, `/user/me`, delegation-token, Homey-login
and six-endpoint read-only inventory operations. The current `remoteUrl` is
cached only from `/user/me`; it is never synthesized. A 401 causes at most one
session recreation per request.

The local provisioning server is fail-closed until a typed local confirmation
is submitted. It supports configuration, direct OAuth callback, Homey listing,
explicit Homey selection and credential wipe. Authorization codes are processed
synchronously by the direct callback and are never persisted.

`app_main` now starts Wi-Fi station mode, waits for an IP address, initializes
NVS, transport and provisioning, then starts the portal. Hardware execution is
still forbidden until the complete offline evidence is reviewed.
