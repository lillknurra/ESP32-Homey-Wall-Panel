#!/usr/bin/env python3
"""Exercise the exact production roller touch callback with a local mock queue."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
source = (component / "panel_ui.c").read_text(encoding="utf-8")
assert "#define ROLLER_FEEDBACK_BLINK_MS 650U" in source
assert "roller_feedback_timer_cb, ROLLER_FEEDBACK_BLINK_MS, ui" in source
assert "if (timer != NULL) lv_timer_delete(timer);" in source
destroy_start = source.index("void panel_ui_destroy(panel_ui_t **ptr)")
destroy_end = source.index("\n}", destroy_start) + 2
assert "roller_feedback_cancel(ui);" in source[destroy_start:destroy_end]
assert "light ? 0xA85D00U : 0xFFD166U" in source
assert "if (!already_pending && pending_mask != 0U)" in source
signature = "static void roller_feedback_zone_event(lv_event_t *event)"
start = source.find(signature)
if start < 0:
    raise SystemExit("production roller touch callback missing")
brace = source.find("{", start)
depth = 0
end = None
for index in range(brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("production roller touch callback body incomplete")
production = source[start:end]

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define LV_EVENT_CLICKED 1
typedef struct panel_ui panel_ui_t;
typedef struct { int code; void *user_data; } lv_event_t;
typedef enum {
    PANEL_UI_ROLLER_ACCEPTED = 0, PANEL_UI_ROLLER_QUEUED_NEXT,
    PANEL_UI_ROLLER_REJECTED_OPPOSITE, PANEL_UI_ROLLER_DEBOUNCED,
    PANEL_UI_ROLLER_LIMIT_REACHED, PANEL_UI_ROLLER_BUSY,
    PANEL_UI_ROLLER_NOT_READY, PANEL_UI_ROLLER_FAILED
} panel_ui_roller_request_result_t;
typedef struct {
    panel_ui_roller_request_result_t (*request_roller_step)(
        void *, size_t, int8_t, uint8_t *, uint8_t *, bool *);
    void *context;
} panel_ui_callbacks_t;
struct panel_ui {
    panel_ui_callbacks_t callbacks;
    void *widget_status[3];
    uint8_t roller_pending_bar_mask[3];
    int8_t roller_active_direction[3];
    uint8_t roller_queued_count[3];
    bool roller_interaction_failed[3];
    bool roller_blink_phase;
};
typedef struct { panel_ui_t *ui; uint8_t widget_index; bool lower_half; } roller_touch_context_t;
static unsigned timer_calls, rejection_calls, render_calls, request_calls;
static bool pending_visible_on_first_render;
static size_t requested_widget;
static int8_t requested_direction;
static panel_ui_roller_request_result_t mock_result = PANEL_UI_ROLLER_ACCEPTED;
static int lv_event_get_code(lv_event_t *event) { return event->code; }
static void *lv_event_get_user_data(lv_event_t *event) { return event->user_data; }
static void roller_feedback_ensure_timer(panel_ui_t *ui) { (void)ui; timer_calls++; }
static bool roller_feedback_has_pending(const panel_ui_t *ui)
{ return ui->roller_pending_bar_mask[1] != 0U || ui->roller_pending_bar_mask[2] != 0U; }
static void roller_feedback_reject(panel_ui_t *ui, uint8_t widget, bool lower)
{ (void)widget; (void)lower; rejection_calls++; roller_feedback_ensure_timer(ui); }
static void render_roller_widget(panel_ui_t *ui, size_t widget)
{
    render_calls++;
    pending_visible_on_first_render =
        ui->roller_active_direction[widget] != 0 &&
        ui->roller_blink_phase && ui->roller_pending_bar_mask[widget] != 0U;
}
#define LV_OBJ_FLAG_HIDDEN 1
static panel_ui_roller_request_result_t request_step(
    void *context, size_t widget, int8_t direction,
    uint8_t *mask, uint8_t *queued, bool *failed)
{
    (void)context; request_calls++; requested_widget = widget;
    requested_direction = direction; *mask = 0x02U; *queued = 1U; *failed = false;
    return mock_result;
}
'''

suffix = r'''
static void exercise(uint8_t widget, bool lower, int8_t expected)
{
    panel_ui_t ui = { .callbacks = { .request_roller_step = request_step } };
    roller_touch_context_t context = { .ui = &ui, .widget_index = widget, .lower_half = lower };
    lv_event_t event = { .code = LV_EVENT_CLICKED, .user_data = &context };
    const unsigned before_request = request_calls;
    mock_result = PANEL_UI_ROLLER_ACCEPTED;
    roller_feedback_zone_event(&event);
    assert(request_calls == before_request + 1U);
    assert(requested_widget == widget && requested_direction == expected);
    assert(timer_calls != 0U && render_calls != 0U);
    assert(ui.roller_pending_bar_mask[widget] == 0x02U);
    assert(ui.roller_blink_phase);
    assert(!pending_visible_on_first_render); /* queueing is not dispatch */
}

int main(void)
{
    exercise(1U, false, -1);
    exercise(1U, true, 1);
    exercise(2U, false, -1);
    exercise(2U, true, 1);
    const unsigned timers_before_reject = timer_calls;
    mock_result = PANEL_UI_ROLLER_REJECTED_OPPOSITE;
    panel_ui_t ui = { .callbacks = { .request_roller_step = request_step },
                      .roller_pending_bar_mask = { 0U, 0x02U, 0U },
                      .roller_queued_count = { 0U, 1U, 0U } };
    roller_touch_context_t opposite = { .ui = &ui, .widget_index = 1U, .lower_half = true };
    lv_event_t rejected = { .code = LV_EVENT_CLICKED, .user_data = &opposite };
    roller_feedback_zone_event(&rejected);
    assert(rejection_calls == 1U);
    assert(timer_calls == timers_before_reject + 1U);
    assert(ui.roller_pending_bar_mask[1] == 0x02U &&
           ui.roller_queued_count[1] == 1U);
    mock_result = PANEL_UI_ROLLER_NOT_READY;
    roller_touch_context_t not_ready = { .ui = &ui, .widget_index = 2U, .lower_half = false };
    lv_event_t refused = { .code = LV_EVENT_CLICKED, .user_data = &not_ready };
    const unsigned before_timer = timer_calls;
    roller_feedback_zone_event(&refused);
    assert(timer_calls == before_timer);
    assert(ui.roller_pending_bar_mask[2] == 0U && ui.roller_queued_count[2] == 0U);
    lv_event_t canceled = { .code = 0, .user_data = &not_ready };
    const unsigned before_request = request_calls;
    roller_feedback_zone_event(&canceled);
    assert(request_calls == before_request);
    puts("PATCH085_AWNING_UI_CALLBACK_TESTS PASS");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temporary:
    cfile = Path(temporary) / "patch085_ui_callback_test.c"
    binary = Path(temporary) / "patch085_ui_callback_test"
    cfile.write_text(prefix + production + "\n" + suffix, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        str(cfile), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
