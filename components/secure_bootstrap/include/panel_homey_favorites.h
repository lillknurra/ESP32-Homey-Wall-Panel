#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "panel_ui_model.h"
#include "panel_homey_alias_provider.h"

#define PANEL_HOMEY_FAVORITE_LIMIT 2U
#define PANEL_HOMEY_FAVORITE_REFERENCE_LIMIT 64U
#define PANEL_HOMEY_FAVORITE_NAME_MAX 96U

typedef enum {
    PANEL_HOMEY_FAVORITES_OK = 0,
    PANEL_HOMEY_FAVORITES_INVALID,
    PANEL_HOMEY_FAVORITES_PARSE_ERROR,
} panel_homey_favorites_result_t;

typedef enum {
    PANEL_HOMEY_FAVORITES_UNVERIFIED = 0,
    PANEL_HOMEY_FAVORITES_VALID_EMPTY,
    PANEL_HOMEY_FAVORITES_VALID_CONFIGURED,
} panel_homey_favorites_state_t;

typedef struct {
    char name[PANEL_HOMEY_FAVORITE_NAME_MAX];
    bool available;
    bool onoff_known;
    bool onoff;
    /* Read-only metadata readiness only. This is not command authorization. */
    bool onoff_command_eligible;
    /* Verified private binding authorization only. This is not execution readiness. */
    bool light_toggle_authorized;
} panel_homey_favorite_public_t;

typedef struct {
    panel_homey_favorite_public_t items[PANEL_HOMEY_FAVORITE_LIMIT];
    size_t count;
    panel_homey_favorites_state_t state;
} panel_homey_favorites_public_t;

/*
 * Publish widget-compatible Favorite Devices from the authoritative Homey
 * contract:
 *   user_json.properties.favoriteDevices[]  (ordered device IDs)
 * joined exactly against devices_json.
 *
 * Raw IDs never leave this private matching layer.
 */
panel_homey_favorites_result_t panel_homey_favorites_parse_and_publish(
    const char *user_json,
    const char *devices_json);

/* Private binding-aware parse path. Raw IDs remain transient and are never published. */
panel_homey_favorites_result_t panel_homey_favorites_parse_and_publish_with_alias_provider(
    const char *user_json,
    const char *devices_json,
    const panel_homey_alias_provider_t *alias_provider);

void panel_homey_favorites_clear(void);
void panel_homey_favorites_revoke_light_toggle_authorization(void);
/*
 * Derived point-in-time execution readiness only. This is not command dispatch
 * authority and performs no Homey mutation. Readiness is never persisted.
 */
bool panel_homey_favorites_light_toggle_execution_ready(
    size_t widget_index,
    bool homey_data_ready);
bool panel_homey_favorites_copy_public(panel_homey_favorites_public_t *output);
panel_homey_favorites_state_t panel_homey_favorites_get_state(void);
const char *panel_homey_favorites_state_name(panel_homey_favorites_state_t state);
bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model);
