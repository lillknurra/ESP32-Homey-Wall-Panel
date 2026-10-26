#include "panel_update_lvgl_psram_pool.h"
#include "panel_update_lvgl_psram_pool_port.h"

#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static jmp_buf fatal_jump;
static int fatal_expected;
static int psram_ready = 1;
static int allocation_mode;
static int alloc_calls;
static int free_calls;
static size_t alloc_size;
static size_t alloc_alignment;
static uint32_t alloc_caps;
static void *allocation_base;

bool panel_update_i5_psram_ready(void) { return psram_ready != 0; }
void *panel_update_i5_aligned_alloc(size_t alignment, size_t bytes, uint32_t caps)
{
    alloc_calls++;
    alloc_alignment = alignment;
    alloc_size = bytes;
    alloc_caps = caps;
    if (allocation_mode == 1) return NULL;
    allocation_base = aligned_alloc(alignment, bytes + (allocation_mode == 2 ? alignment : 0U));
    assert(allocation_base != NULL);
    if (allocation_mode == 2) return (uint8_t *)allocation_base + 1;
    return allocation_base;
}
void panel_update_i5_free(void *ptr)
{
    (void)ptr;
    free_calls++;
    free(allocation_base);
    allocation_base = NULL;
}
bool panel_update_i5_is_external_ram(const void *ptr)
{
    if (allocation_mode == 3) return false;
    return ptr == allocation_base;
}
_Noreturn void panel_update_i5_fatal(const char *reason)
{
    (void)reason;
    if (fatal_expected) longjmp(fatal_jump, 1);
    abort();
}

static void expect_fatal(void (*fn)(void))
{
    fatal_expected = 1;
    if (setjmp(fatal_jump) == 0) {
        fn();
        assert(!"fatal gate returned");
    }
    fatal_expected = 0;
}

static void wrong_size(void) { (void)panel_update_lvgl_psram_pool_alloc(65535U); }
static void no_psram(void) { psram_ready = 0; (void)panel_update_lvgl_psram_pool_alloc(65536U); }
static void allocation_failure(void) { allocation_mode = 1; (void)panel_update_lvgl_psram_pool_alloc(65536U); }
static void invalid_alignment(void) { allocation_mode = 2; (void)panel_update_lvgl_psram_pool_alloc(65536U); }
static void internal_only(void) { allocation_mode = 3; (void)panel_update_lvgl_psram_pool_alloc(65536U); }

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "wrong-size") == 0) { expect_fatal(wrong_size); return 0; }
    if (strcmp(argv[1], "no-psram") == 0) { expect_fatal(no_psram); assert(alloc_calls == 0); return 0; }
    if (strcmp(argv[1], "alloc-fail") == 0) { expect_fatal(allocation_failure); assert(alloc_calls == 1); return 0; }
    if (strcmp(argv[1], "misaligned") == 0) { expect_fatal(invalid_alignment); assert(free_calls == 1); return 0; }
    if (strcmp(argv[1], "internal") == 0) { expect_fatal(internal_only); assert(free_calls == 1); return 0; }
    assert(strcmp(argv[1], "success") == 0);
    void *pool = panel_update_lvgl_psram_pool_alloc(65536U);
    assert(pool != NULL);
    assert(((uintptr_t)pool % 16U) == 0U);
    assert(alloc_size == 65536U && alloc_alignment == 16U);
    assert(alloc_caps == ((1U << 10) | (1U << 2)));
    assert(panel_update_lvgl_psram_pool_alloc(65536U) == pool);
    assert(alloc_calls == 1); /* repeated LVGL init reuses the stable pool */
    assert(free_calls == 0);
    free(allocation_base);
    return 0;
}
