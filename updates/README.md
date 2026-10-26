# Central firmware update metadata (NOT YET PUBLISHED)

The OTA manifest is a static HTTPS file per channel and hardware model, e.g.

- `updates/stable/waveshare-esp32-s3-touch-lcd-4b-n16r8.json`
- `updates/beta/waveshare-esp32-s3-touch-lcd-4b-n16r8.json`

**No deployable JSON file ships with Patch116I.** Never publish a placeholder as if it were an actual signed release.
Generate one from a cryptographically verified, operator-approved signed firmware binary using `tools/ota/panel_release_manifest.py`. Validate before a separately authorized public GitHub Pages/CDN publication. The serving hostname/location must be verified before any firmware is configured to poll it.

These JSON files are read-only availability metadata, not authorization to install firmware. Normal OTA signing, rollback and per-device authorization remain mandatory.
