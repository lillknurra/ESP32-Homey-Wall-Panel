#include "panel_ui_layout.h"
#include <assert.h>
#include <stdio.h>

static int overlaps(const panel_ui_widget_layout_t *a,
                    const panel_ui_widget_layout_t *b)
{
    return a->x < b->x + b->width && b->x < a->x + a->width &&
           a->y < b->y + b->height && b->y < a->y + a->height;
}

int main(void)
{
    unsigned counts[PANEL_UI_PAGE_COUNT] = {0};
    const unsigned expected_page[6] = {1, 0, 0, 2, 0, 0};
    assert(PANEL_UI_PAGE_COUNT == 3 && PANEL_UI_WIDGET_COUNT == 6);
    for (unsigned i = 0; i < PANEL_UI_WIDGET_COUNT; ++i) {
        const panel_ui_widget_layout_t *a = &PANEL_UI_WIDGET_LAYOUT[i];
        assert(a->page == expected_page[i]);
        counts[a->page]++;
        assert(a->x >= 0 && a->y >= 0);
        assert(a->x + a->width <= PANEL_UI_LAYOUT_WIDTH);
        assert(a->y + a->height <= PANEL_UI_LAYOUT_PAGE_HEIGHT);
        for (unsigned j = 0; j < i; ++j) {
            if (a->page == PANEL_UI_WIDGET_LAYOUT[j].page)
                assert(!overlaps(a, &PANEL_UI_WIDGET_LAYOUT[j]));
        }
    }
    assert(counts[0] == 4 && counts[1] == 1 && counts[2] == 1);
    assert(PANEL_UI_WIDGET_LAYOUT[1].x < PANEL_UI_WIDGET_LAYOUT[2].x);
    assert(PANEL_UI_WIDGET_LAYOUT[4].y > PANEL_UI_WIDGET_LAYOUT[1].y);
    assert(PANEL_UI_WIDGET_LAYOUT[5].y > PANEL_UI_WIDGET_LAYOUT[2].y);
    for (unsigned i = 0; i < 3; ++i) {
        assert(PANEL_UI_MARKIS_CONTROL_WIDTH >= 44);
        assert(PANEL_UI_MARKIS_CONTROL_HEIGHT >= 44);
        assert(PANEL_UI_MARKIS_CONTROL_X[i] >= 1);
        assert(PANEL_UI_MARKIS_CONTROL_X[i] + PANEL_UI_MARKIS_CONTROL_WIDTH <
               PANEL_UI_WIDGET_LAYOUT[0].width - 1);
        assert(PANEL_UI_MARKIS_CONTROL_Y + PANEL_UI_MARKIS_CONTROL_HEIGHT <
               PANEL_UI_WIDGET_LAYOUT[0].height - 1);
        if (i != 0)
            assert(PANEL_UI_MARKIS_CONTROL_X[i-1] + PANEL_UI_MARKIS_CONTROL_WIDTH <
                   PANEL_UI_MARKIS_CONTROL_X[i]);
    }
    /* Six menu buttons followed by feedback, including all six row gaps. */
    assert(PANEL_UI_SETTINGS_BUTTON_HEIGHT >= 44);
    const unsigned body_height = 6 * PANEL_UI_SETTINGS_BUTTON_HEIGHT +
        6 * PANEL_UI_SETTINGS_ROW_GAP + PANEL_UI_SETTINGS_FEEDBACK_HEIGHT;
    assert(body_height <= PANEL_UI_SETTINGS_CONTENT_HEIGHT);
    assert(76 + PANEL_UI_SETTINGS_CONTENT_HEIGHT <= 480);
    puts("PATCH076_V28_LAYOUT_GEOMETRY PASS");
    return 0;
}
