#include "panel_ui_model.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    panel_ui_model_t model = {0};
    panel_ui_model_init(&model, 1U);
    panel_ui_open_settings_model(&model);
    panel_ui_request_confirmation(&model, PANEL_CONFIRM_FIRMWARE_UPDATE);
    assert(model.view == PANEL_UI_VIEW_CONFIRMATION);
    assert(panel_ui_accept_confirmation(&model) == PANEL_CONFIRMED_ACTION_FIRMWARE_UPDATE);
    assert(model.view == PANEL_UI_VIEW_SETTINGS);
    assert(model.confirmation == PANEL_CONFIRM_NONE);

    panel_ui_request_confirmation(&model, PANEL_CONFIRM_FIRMWARE_UPDATE);
    panel_ui_cancel_confirmation(&model);
    assert(model.view == PANEL_UI_VIEW_SETTINGS);
    assert(panel_ui_accept_confirmation(&model) == PANEL_CONFIRMED_ACTION_NONE);
    puts("PATCH120B_PRODUCTION_UI_CONFIRMATION=PASS");
}
