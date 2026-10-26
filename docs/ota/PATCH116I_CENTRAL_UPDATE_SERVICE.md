# Patch116I - Central firmware update discovery (source-only/offline)

Date: 2026-10-25. Baseline reference: `origin/main` at `148ecda00ce79bd906ef67e63a24dc7e0f7658e8` (verify live ref again in Codex before use).

## Product goal

Many independent ESP32-S3 Homey panels, at different locations and behind different Wi-Fi networks, check one public update source and independently discover a new firmware release. They do **not** require a shared LAN, the operator's MacBook, or contact with Homey to check versions.

**Patch116I delivers the central static manifest protocol, offline generation tool, strict firmware-side parser, release comparison and read-only scheduling policy. It does NOT yet deliver network polling, on-screen notification, signed firmware installation, OTA partitions, a running server, a release publication, or an ESP-IDF build.**

## Distribution architecture

- Static, public, HTTPS manifest endpoint per hardware model and release channel:
  `https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/stable/waveshare-esp32-s3-touch-lcd-4b-n16r8.json` (PROPOSED example, *not verified as deployed*).
- GitHub Pages can host the small read-only JSON files (after separately configuring publication); an independent public static host/CDN can replace it later.
- Signed firmware assets hosted using GitHub Releases. Each version has an immutable release tag `panel-vX.Y.Z` for stable and `panel-beta-vX.Y.Z` for beta. The artifact filename is `esp32_homey_wall_panel_<hardware_model>_<version>_signed.bin`.
- The publisher creates files locally, after independent signature verification. This patch never creates a release or modifies GitHub or any device.
- A panel checks the *pinned manifest URL* read-only, after Wi-Fi is connected and normal boot has settled; no dependency on Homey READY. Use a low-priority worker with bounded HTTPS transport and **no write-side effects**.
- Default manual-install policy for eventual OTA: the panel displays availability but does not automatically download/install. User consent and a separate safe OTA maintenance boot are required for installing signed images.

## Manifest JSON schema v1

Exactly eight root keys and exactly three image keys (unknown or duplicate keys rejected):

```json
{
  "schema_version": 1,
  "hardware_model": "waveshare-esp32-s3-touch-lcd-4b-n16r8",
  "project": "esp32_homey_wall_panel",
  "channel": "stable",
  "release_sequence": 120,
  "version": "1.2.0",
  "image": {
    "size_bytes": 1749280,
    "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "url": "https://github.com/lillknurra/ESP32-Homey-Wall-Panel/releases/download/panel-v1.2.0/esp32_homey_wall_panel_waveshare-esp32-s3-touch-lcd-4b-n16r8_1.2.0_signed.bin"
  },
  "notes": "Bugfixes and improvements"
}
```

**This is a non-release example with a placeholder digest and unverified URL. DO NOT PUBLISH it.**

Rules: full body <=2048 bytes; explicit schema 1; stable/beta channel; exact model/project; semantic X.Y.Z version; positive globally monotonic release_sequence (not Git commit count); 288 <= signed image bytes <= 4 MiB (preliminary slot policy); canonical lower-case SHA-256; canonical GitHub Releases URL with no query, fragment or userinfo; notes <=159 UTF-8 bytes, no controls. Future schema must be explicitly versioned, not silently accepted.

Installed local firmware needs independently compiled `installed_release_sequence` and semantic version. The present Patch113 and default-off Patch116 candidates have Git-hash versions without an authoritative release sequence; they must be reported as `UNVERSIONED`, never compared lexicographically to semver. An OTA migration/release must establish both after proper signing.

## Device check policy

- First check: 90 seconds after initial scheduled time + 0-10 minute spread (caller supplies nonidentifying seed); never at boot splash.
- Regular check: 24 hours after previous successful check.
- Error retry: after 15 minutes.
- User-triggered check: permitted outside the normal interval, only after Wi-Fi and normal UI boot have settled.
- One outstanding request; stale generation responses ignored. A 30-second **logical** deadline expires the read-only status, **but is not proof that a third-party DNS/TLS function has returned or that its callback may be freed**. The network adapter must separately own and bound I/O lifecycle.
- HTTPS must verify CA/certificate and the exact fixed manifest origin. Do not accept arbitrary user-provided manifest URL, HTTP fallback, or automatic cross-origin redirects. Cache headers/ETags may be added later, without changing trust rules.
- A notification is never a firmware command. `PANEL_UPDATE_NEWER_AVAILABLE` authorizes only rendering informational UI.
- No device credentials are sent to the manifest endpoint. Fleet traffic naturally exposes client IP to the hosting provider; avoid logging identifiers or Homey data.
- Protect startup responsiveness, internal heap and RGB: network discovery must be delayed/low priority and must not write any flash. Homey refresh/control, awning 2/3=1/1, awning 1 disabled, 12 MHz PCLK and RGB cacheline 64 remain unchanged.

## Release publication order (separate operator authorization)

1. Accept a clean source/build and assign release sequence and semantic version (e.g. 120, 1.2.0) to the firmware image itself.
2. Build with ESP-IDF 6.0.1 and the correct hardware profile, sign the image with operator-controlled private key **outside the repository**. Verify signature using the established trust anchor; the manifest publisher cannot do this.
3. Check image identity, signature, size, SHA-256, compatible OTA slot, NVS preservation and rollback/migration plan.
4. Publish immutable signed `.bin` under the correct version tag in a public distribution repository's GitHub Releases (if source repo is private, use a separate public distribution host). Verify anonymous HTTPS download and any redirects.
5. Run local `panel_release_manifest.py create ... --operator-verified-signature` to calculate exact bytes/SHA and output the draft manifest (this flag is a declaration, not cryptographic proof).
6. Validate generated JSON with both Python publisher and ESP32 parser, then publish it to GitHub Pages only **after** the release file is available. No automated publication is included in this patch.
7. Verify manifest accessible anonymously over HTTPS, validate cache behaviour, and test stable/beta/model selections.
8. Existing factory-only devices need an independently approved one-time USB migration: new bootloader, partition table, signed starting app, OTA slot setup and recovery procedure. Never assume OTA can install itself on the current Patch113 layout.

Rollback of a bad *published release* should use a newly signed corrected version with a **new larger release sequence**. Do not silently move version metadata backwards. The future installer must verify image signature and identity and compare with running version independently of this untrusted read-only manifest.

## Boundaries and evidence

- Implemented and tested offline: host C metadata validator + strict cJSON reader + read-only scheduler, Python release manifest generator/validator, deterministic tests.
- Not implemented: HTTPS client, UI notification, scheduling task hookup, public Pages host, release download, private key storage/signing, app-first-boot confirmation, installer, physical OTA.
- `PANEL_UPDATE_NEWER_AVAILABLE` is **not** authorization to flash, write otadata, reboot, control Homey or move awnings.
- The prior Patch116F/G pull-OTA approach still has unresolved synchronous DNS/TLS deadline issues; this patch does not hide or solve them. Read-only checking can be developed independently. Keep the prior unsafe updater compile-time blocked.
- No changes to `partitions.csv`, `sdkconfig`, `sdkconfig.defaults`, original checkout, existing source, external accounts or GitHub refs.

## Next verified work

Codex: apply in isolated worktree; host tests; ESP-IDF v6.0.1 native compile, cJSON ABI/CMake check, default-off firmware build; preserve active factory layout, 1/1 awning defaults and display profile. After build evidence, next separate patch for a **read-only HTTPS manifest fetch adapter** with bounded ownership and UI announcement. The installer is later, after signed release / OTA migration / full rollback verification.
