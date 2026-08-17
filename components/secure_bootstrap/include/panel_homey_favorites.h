#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "panel_ui_model.h"

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

void panel_homey_favorites_clear(void);
bool panel_homey_favorites_copy_public(panel_homey_favorites_public_t *output);
panel_homey_favorites_state_t panel_homey_favorites_get_state(void);
const char *panel_homey_favorites_state_name(panel_homey_favorites_state_t state);
bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model);
