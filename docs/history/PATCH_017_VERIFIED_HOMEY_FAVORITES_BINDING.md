# Patch 017 - Verified Homey Favorites Binding

## Status

- Status: `COMPLETE / MERGED`
- Stable commit on `main`: `253319f361b9967ebcaca376591bd14ecf3d9c0e`
- Commit title: `Patch 017: bind verified Homey favorites with resilient readiness`

## Purpose

Bind verified Homey favorites to the panel's read-only favorite widgets using
authoritative Homey favorite ordering and resilient readiness behavior.

## Functional result

Patch017:

- uses authoritative user `favoriteDevices` ordering;
- gates dashboard publication on verified live data;
- retries transient Homey transport failures;
- preserves favorite widget state across wake;
- cleans up refresh ownership and legacy discovery behavior;
- preserves the read-only dashboard boundary.

## Merged scope

The merge commit changed these files:

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/include/athom_oauth_runtime.h`;
- `components/secure_bootstrap/include/panel_homey_favorites.h`;
- `components/secure_bootstrap/panel_homey_favorites.c`;
- `components/secure_bootstrap/panel_ui.c`;
- `components/secure_bootstrap/panel_ui_model.c`;
- `components/secure_bootstrap/secure_bootstrap_esp.c`;
- `components/secure_bootstrap/test_host/test_panel_homey_favorites.c`.

## Boundaries preserved

Patch017 does not authorize:

- Homey mutation;
- Package 3B;
- Patch013 runtime closure;
- raw Homey identifiers in Git, UI, logs or evidence;
- generic device-control UI;
- build-system or sdkconfig policy changes outside its merged implementation
  evidence.

## Documentation note

Patch020 adds this historical record after Patch017 was already present on
`main`. The durable Patch020 reconciliation does not alter Patch017 code or make
new runtime claims.
