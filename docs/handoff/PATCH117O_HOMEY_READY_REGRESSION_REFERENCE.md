# Patch117O Homey READY regression comparison reference

Date: 2026-11-01

## Source and installation status

- Last verified development source: Patch117M, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`.
- Installed firmware: Patch117M, according to the operator and retained flash evidence. The separately recorded BIN SHA-256 is `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`.
- Patch117O is a documentation-only comparison checkpoint. It has no functional firmware change, and no rollback has been selected. The objective is to repair the latest firmware.
- Canonical local `main` remains `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; this documentation branch does not modify it.

## Public comparison snapshot

The comparison branch is a one-commit file-tree snapshot based on the already public `origin/main` parent; it does not carry Patch117M, Patch117O, or private handoff-reconciliation commits as ancestors. The snapshot preserves Patch117M firmware source byte-for-byte. Path-only sanitization was limited to documentation and non-firmware helper/test paths. No functional correction is included, and the installed firmware remains Patch117M. The purpose is a ChatGPT-led regression comparison against Patch113 and the verifiable Patch116I11 source.

## Current Homey evidence

The operator reports that Wi-Fi works while Homey READY does not reliably occur. In the retained Patch117M runtime evidence, the first BOOT_AUTO Favorites request returned HTTP 401. The bounded recovery path was then followed by a later Favorites HTTP 200. The subsequent Devices request failed before verified response headers:

- `ESP_ERR_HTTP_FETCH_HEADER` = 28676 (`0x6A04`)
- `ESP_ERR_MBEDTLS_SSL_READ_FAILED` = 32797 (`0x801D`)
- underlying TLS result = `-132`
- Devices response headers/status/body were not verified; no snapshot was published by that attempt.

The available evidence does not establish why the TLS read failed. Internal memory exhaustion is not proven. TLS connection reuse is a regression candidate, not a proven cause. No safe functional correction has been identified. The Patch117N HTTPD body-stall and Wi-Fi timing investigations did not prove a shared cause and are parked.

These are operator/runtime observations recorded in the existing evidence, not new physical verification performed by this documentation checkpoint.

## Comparison references

### Patch116I11

- Registered source base: `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`.
- The archived I11 package identifies the build as `148ecda-dirty` and includes BIN/ELF, build configuration/manifests, and a `SOURCE_STATE_SHA256SUMS.txt` with 148 path/hash entries.
- The archive does not include the source files or a unified source diff for the dirty overlay. A checksum manifest identifies expected file contents but cannot reproduce missing contents. Therefore `PATCH116I11_EXACT_SOURCE_AVAILABLE=NO` for the complete physical build source and `PATCH116I11_EXACT_SOURCE_DIFF_PATH=NOT_VERIFIED`.
- The exact list of dirty file contents is not available from the archive alone. The base commit must not be presented as an exact representation of the physical I11 firmware.
- Do not reconstruct source from assumptions or publish I11 as a new branch based on this package.

### Patch113

- Source reference: `6ae35162bb17d99f80178c3632be7f387f95ab31`.
- This is the prior production baseline reference requested for ChatGPT's later comparison. No comparative regression conclusion is made in this document.

## Next analysis

After the checkpoint is safely published, ChatGPT will compare Patch117O with Patch116I11's verifiable source evidence and Patch113. This checkpoint makes no claim that a particular commit caused the Homey failures. No functional fix, rollback, or physical action is authorized by this document.
