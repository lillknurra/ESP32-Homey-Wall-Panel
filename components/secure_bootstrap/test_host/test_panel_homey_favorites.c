#include "panel_homey_favorites.h"
#include "panel_ui_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void publish_and_apply(const char *json, panel_ui_model_t *model)
{
    assert(panel_homey_favorites_parse_and_publish(json) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_apply_ui_model(model));
}

static void expect_text(const panel_ui_model_t *model, size_t widget, const char *expected)
{
    char buffer[32];
    assert(panel_ui_widget_display_text(model, widget, buffer, sizeof(buffer)));
    assert(strcmp(buffer, expected) == 0);
}

static void test_zero(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    publish_and_apply("[]", &model);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 4U), "Belysning 1") == 0);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 5U), "Belysning 2") == 0);
    expect_text(&model, 4U, "Ej konfigurerad");
    expect_text(&model, 5U, "Ej konfigurerad");
}

static void test_one(void)
{
    static const char json[] =
        "[{\"name\":\"Hall\",\"favorite\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}]";
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    publish_and_apply(json, &model);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 4U), "Hall") == 0);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 5U), "Belysning 2") == 0);
    expect_text(&model, 4U, "Tänd");
    expect_text(&model, 5U, "Ej konfigurerad");
}

static void test_two(void)
{
    static const char json[] =
        "[{\"name\":\"Hall\",\"favorite\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},"
        "{\"name\":\"Kök\",\"favorite\":true,\"available\":false,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}]";
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    publish_and_apply(json, &model);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 4U), "Hall") == 0);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 5U), "Kök") == 0);
    expect_text(&model, 4U, "Släckt");
    expect_text(&model, 5U, "Otillgänglig");
}

static void test_more_than_two(void)
{
    static const char json[] =
        "[{\"name\":\"A\",\"favorite\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":true}}},"
        "{\"name\":\"B\",\"favorite\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},"
        "{\"name\":\"C\",\"favorite\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}]";
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    publish_and_apply(json, &model);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 4U), "A") == 0);
    assert(strcmp(panel_ui_widget_title_for_model(&model, 5U), "B") == 0);
    expect_text(&model, 4U, "Tänd");
    expect_text(&model, 5U, "Släckt");
}

int main(void)
{
    test_zero();
    test_one();
    test_two();
    test_more_than_two();
    puts("PANEL_HOMEY_FAVORITES_UI_WIRING_TEST PASS");
    return 0;
}
