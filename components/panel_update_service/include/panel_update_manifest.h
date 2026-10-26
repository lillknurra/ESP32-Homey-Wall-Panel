/* Patch116I: read-only central firmware availability metadata, never an installer. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_UPDATE_MANIFEST_SCHEMA 1U
#define PANEL_UPDATE_MANIFEST_MAX_BYTES 2048U
#define PANEL_UPDATE_MAX_IMAGE_BYTES UINT32_C(0x400000)
#define PANEL_UPDATE_MODEL_ID "waveshare-esp32-s3-touch-lcd-4b-n16r8"
#define PANEL_UPDATE_PROJECT_ID "esp32_homey_wall_panel"
#define PANEL_UPDATE_RELEASE_PREFIX "https://github.com/lillknurra/ESP32-Homey-Wall-Panel/releases/download/"

#define PANEL_UPDATE_MODEL_SIZE 64U
#define PANEL_UPDATE_PROJECT_SIZE 48U
#define PANEL_UPDATE_CHANNEL_SIZE 12U
#define PANEL_UPDATE_VERSION_SIZE 32U
#define PANEL_UPDATE_URL_SIZE 384U
#define PANEL_UPDATE_NOTES_SIZE 160U

typedef struct {
    uint32_t schema_version;
    char hardware_model[PANEL_UPDATE_MODEL_SIZE];
    char project[PANEL_UPDATE_PROJECT_SIZE];
    char channel[PANEL_UPDATE_CHANNEL_SIZE];
    uint32_t release_sequence;
    char version[PANEL_UPDATE_VERSION_SIZE];
    uint32_t image_size_bytes;
    uint8_t image_sha256[32];
    char image_url[PANEL_UPDATE_URL_SIZE];
    char notes[PANEL_UPDATE_NOTES_SIZE];
} panel_update_manifest_t;

typedef struct {
    const char *hardware_model;
    const char *project;
    const char *channel;
    uint32_t installed_release_sequence;
    const char *installed_version;
    bool has_ota_partitions;
    uint32_t inactive_ota_capacity;
} panel_update_local_t;

typedef enum {
    PANEL_UPDATE_REJECTED = 0,               /* malformed, mismatched, untrusted for display */
    PANEL_UPDATE_UNVERSIONED = 1,            /* no installed release sequence */
    PANEL_UPDATE_CURRENT = 2,                /* no newer update */
    PANEL_UPDATE_NEWER_NEEDS_MIGRATION = 3,  /* metadata only: no OTA layout */
    PANEL_UPDATE_NEWER_AVAILABLE = 4,        /* metadata only: never authorizes install */
    PANEL_UPDATE_NEWER_TOO_LARGE = 5,        /* metadata only: cannot fit OTA slot */
} panel_update_status_t;

/* Strict JSON schema v1: rejects duplicate/unknown fields, invalid types and trailing data.
 * Successful parse is NOT cryptographic authentication of manifest or firmware.
 */
bool panel_update_manifest_parse(const char *json, size_t json_length,
                                 panel_update_manifest_t *out);

/* Pure policy: never performs a network request, Homey call, flash write or reboot. */
panel_update_status_t panel_update_manifest_evaluate(
    const panel_update_manifest_t *manifest, const panel_update_local_t *local);

/* Utility for release tooling and tests; does not validate a signature. */
bool panel_update_manifest_metadata_valid(const panel_update_manifest_t *manifest);
