#define _POSIX_C_SOURCE 200809L

#include "panel_clock.h"
#include "panel_ui_model.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static time_t utc_epoch(int year, int month, int day, int hour, int minute)
{
    struct tm value = {
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_isdst = 0,
    };
    assert(setenv("TZ", "UTC0", 1) == 0);
    tzset();
    return mktime(&value);
}

static struct tm stockholm_time(time_t timestamp)
{
    struct tm value;
    assert(localtime_r(&timestamp, &value) != NULL);
    return value;
}

static void test_timezone_and_dst(void)
{
    assert(panel_clock_configure_timezone());
    assert(strcmp(getenv("TZ"), PANEL_CLOCK_TZ_RULE) == 0);

    const time_t spring_before = utc_epoch(2026, 3, 29, 0, 59);
    assert(panel_clock_configure_timezone());
    struct tm local = stockholm_time(spring_before);
    assert(local.tm_hour == 1 && local.tm_min == 59 && local.tm_isdst == 0);
    local = stockholm_time(spring_before + 60);
    assert(local.tm_hour == 3 && local.tm_min == 0 && local.tm_isdst > 0);

    const time_t autumn_before = utc_epoch(2026, 10, 25, 0, 59);
    assert(panel_clock_configure_timezone());
    local = stockholm_time(autumn_before);
    assert(local.tm_hour == 2 && local.tm_min == 59 && local.tm_isdst > 0);
    local = stockholm_time(autumn_before + 60);
    assert(local.tm_hour == 2 && local.tm_min == 0 && local.tm_isdst == 0);
}

static void test_swedish_format_and_midnight(void)
{
    const time_t before_midnight = utc_epoch(2026, 6, 1, 21, 59);
    assert(panel_clock_configure_timezone());
    struct tm local = stockholm_time(before_midnight);
    char clock_text[8];
    char date_text[40];
    assert(panel_ui_format_clock(true, &local, clock_text, sizeof(clock_text)));
    assert(strcmp(clock_text, "23:59") == 0);
    assert(panel_ui_format_date_sv(true, &local, date_text, sizeof(date_text)));
    assert(strcmp(date_text, "mån 1 juni") == 0);

    local = stockholm_time(before_midnight + 60);
    assert(panel_ui_format_clock(true, &local, clock_text, sizeof(clock_text)));
    assert(strcmp(clock_text, "00:00") == 0);
    assert(panel_ui_format_date_sv(true, &local, date_text, sizeof(date_text)));
    assert(strcmp(date_text, "tis 2 juni") == 0);
}

static void test_missing_time_and_minute_updates(void)
{
    panel_clock_render_state_t state = {0};
    assert(!panel_clock_epoch_is_valid((time_t)0));
    assert(panel_clock_render_is_due(&state, (time_t)0, false));
    panel_clock_render_committed(&state, (time_t)0, false);
    assert(!panel_clock_render_is_due(&state, (time_t)1, false));

    const time_t synchronized = PANEL_CLOCK_MIN_VALID_UNIX_SECONDS + 42;
    assert(panel_clock_render_is_due(&state, synchronized, true));
    panel_clock_render_committed(&state, synchronized, true);
    assert(!panel_clock_render_is_due(&state, synchronized + 17, true));
    assert(panel_clock_render_is_due(&state, synchronized + 60, true));
    panel_clock_render_committed(&state, synchronized + 60, true);
    assert(!panel_clock_render_is_due(&state, synchronized + 61, true));
}

int main(void)
{
    test_timezone_and_dst();
    test_swedish_format_and_midnight();
    test_missing_time_and_minute_updates();
    puts("PANEL_CLOCK_TESTS PASS");
    return 0;
}
