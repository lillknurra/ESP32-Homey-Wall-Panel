#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "panel_ui_model.h"
#define PANEL_HOMEY_FAVORITE_LIMIT 2U
#define PANEL_HOMEY_FAVORITE_NAME_MAX 96U
typedef enum { PANEL_HOMEY_FAVORITES_OK=0, PANEL_HOMEY_FAVORITES_INVALID, PANEL_HOMEY_FAVORITES_PARSE_ERROR } panel_homey_favorites_result_t;
typedef struct { char name[PANEL_HOMEY_FAVORITE_NAME_MAX]; bool available; bool onoff_known; bool onoff; } panel_homey_favorite_public_t;
typedef struct { panel_homey_favorite_public_t items[PANEL_HOMEY_FAVORITE_LIMIT]; size_t count; } panel_homey_favorites_public_t;
panel_homey_favorites_result_t panel_homey_favorites_parse_and_publish(const char *devices_json);
bool panel_homey_favorites_copy_public(panel_homey_favorites_public_t *output);

bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model);
