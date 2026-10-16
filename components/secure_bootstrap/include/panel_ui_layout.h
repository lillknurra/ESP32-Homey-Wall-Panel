#pragma once

#include "panel_ui_model.h"

/* Visual placement only: array indices remain the six Homey widget indices. */
typedef struct {
    uint8_t page;
    int16_t x, y, width, height;
} panel_ui_widget_layout_t;

#define PANEL_UI_LAYOUT_WIDTH 480
#define PANEL_UI_LAYOUT_PAGE_HEIGHT 392
#define PANEL_UI_FAVORITE_CARD_WIDTH 220
#define PANEL_UI_FAVORITE_CARD_HEIGHT 176
#define PANEL_UI_MARKIS_CONTROL_WIDTH 112
#define PANEL_UI_MARKIS_CONTROL_HEIGHT 112
#define PANEL_UI_MARKIS_CONTROL_Y 104
#define PANEL_UI_SETTINGS_BUTTON_HEIGHT 52
#define PANEL_UI_SETTINGS_ROW_GAP 6
#define PANEL_UI_SETTINGS_FEEDBACK_HEIGHT 32
#define PANEL_UI_SETTINGS_CONTENT_HEIGHT 392

static const panel_ui_widget_layout_t PANEL_UI_WIDGET_LAYOUT[PANEL_UI_WIDGET_COUNT] = {
    {1, 32, 36, 416, 320}, /* Markis */
    {0, 14, 12, PANEL_UI_FAVORITE_CARD_WIDTH, PANEL_UI_FAVORITE_CARD_HEIGHT},
    {0, 246, 12, PANEL_UI_FAVORITE_CARD_WIDTH, PANEL_UI_FAVORITE_CARD_HEIGHT},
    {2, 32, 76, 416, 240}, /* Verisure */
    {0, 14, 204, PANEL_UI_FAVORITE_CARD_WIDTH, PANEL_UI_FAVORITE_CARD_HEIGHT},
    {0, 246, 204, PANEL_UI_FAVORITE_CARD_WIDTH, PANEL_UI_FAVORITE_CARD_HEIGHT},
};

static const int16_t PANEL_UI_MARKIS_CONTROL_X[3] = {20, 152, 284};
