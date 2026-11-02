# Patch117Q physical validation summary

Date: 2026-11-01

## Candidate and experiment mode

- Source candidate: Patch117Q, commit `4f111dd423d4e4b695ebb87784ebd698bfc8990c`, tree `163df1648a7f2f0cde80441bd2ef09c90278b7c3` before this public snapshot commit.
- Installed test image: Patch117Q app-only image; BIN SHA-256 `e2835699e308b5b1f2e561c15cecc9024da672f319b77e78960c42955284739f`.
- `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT` defaults to OFF. The physically tested candidate build explicitly had it ON. Future builds must not be assumed to contain this experiment unless their effective build configuration confirms ON.
- The experiment closes the Homey transport after successful Zones and before the single Devices GET. It is not established as a permanent fix.

## Physical result

The operator-authorized app-only flash completed at the factory app offset and esptool verified the written image hash. The normal RTS reset followed. No manual power cycle, Homey mutation or awning movement was performed.

One bounded read-only status GET returned HTTP 200 with valid JSON. At that observation the panel reported `state=ready`, `homey_data_state=ready`, 19 zones and 79 devices. The current awning snapshot was present, fresh and generation-valid. The operator separately reports video showing the dashboard with status “Ansluten”; no raw video or device-identifying data is included here.

The Devices HTTP capture recorded a complete 313,761-byte HTTP 200 response. The HTTP client handle was reused, while the underlying connection was reported as not reused with one successful connect call. This supports that a fresh connection was used for a Devices request in this runtime. The capture's request sequence is an HTTP-perform identifier and is not directly correlated to the inventory sequence in the status response.

The latest inventory record in the single status response was periodic sequence 3, completed successfully. The immutable first BOOT_AUTO result was not read in this test. Therefore first-attempt success and READY through the first BOOT_AUTO attempt remain NOT VERIFIED. This observation does not prove long-term reliability or root cause.

## Safety and evidence boundary

No firmware source, Homey data, credentials, raw runtime log, serial identifier or video was added. The report distinguishes the physical flash, the single runtime observation, and the still-unverified first-attempt outcome.
