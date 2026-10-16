#!/usr/bin/env python3
"""Exercise the exact production roller touch callback with a local mock queue."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
source = (component / "panel_ui.c").read_text(encoding="utf-8")
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
typedef struct {
    int (*request_roller_step)(void *, size_t, int8_t);
    void *context;
} panel_ui_callbacks_t;
struct panel_ui { panel_ui_callbacks_t callbacks; };
typedef struct { panel_ui_t *ui; uint8_t widget_index; bool lower_half; } roller_touch_context_t;
static unsigned feedback_calls, request_calls;
static uint8_t feedback_widget;
static bool feedback_lower;
static size_t requested_widget;
static int8_t requested_direction;

static int lv_event_get_code(lv_event_t *event) { return event->code; }
static void *lv_event_get_user_data(lv_event_t *event) { return event->user_data; }
static void roller_feedback_start(panel_ui_t *ui, uint8_t widget, bool lower)
{ (void)ui; feedback_calls++; feedback_widget = widget; feedback_lower = lower; }
static int request_step(void *context, size_t widget, int8_t direction)
{ (void)context; request_calls++; requested_widget = widget; requested_direction = direction; return 0; }

'''
suffix = r'''
static void exercise(uint8_t widget, bool lower, int8_t expected)
{
    panel_ui_t ui = { .callbacks = { .request_roller_step = request_step, .context = NULL } };
    roller_touch_context_t context = { .ui = &ui, .widget_index = widget, .lower_half = lower };
    lv_event_t event = { .code = LV_EVENT_CLICKED, .user_data = &context };
    const unsigned before_feedback = feedback_calls;
    const unsigned before_request = request_calls;
    roller_feedback_zone_event(&event);
    assert(feedback_calls == before_feedback + 1U);
    assert(feedback_widget == widget && feedback_lower == lower);
    if (widget == 1U || widget == 2U) {
        assert(request_calls == before_request + 1U);
        assert(requested_widget == widget && requested_direction == expected);
    } else {
        assert(request_calls == before_request);
    }
}

int main(void)
{
    exercise(1U, false, -1);
    exercise(1U, true, 1);
    exercise(2U, false, -1);
    exercise(2U, true, 1);
    panel_ui_t ui = { .callbacks = { .request_roller_step = request_step } };
    roller_touch_context_t context = { .ui = &ui, .widget_index = 1U, .lower_half = false };
    lv_event_t canceled = { .code = 0, .user_data = &context };
    const unsigned before_feedback = feedback_calls, before_request = request_calls;
    roller_feedback_zone_event(&canceled);
    assert(feedback_calls == before_feedback && request_calls == before_request);
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
