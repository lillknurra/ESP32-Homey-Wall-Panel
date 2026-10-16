#include "panel_render_diag.h"

/* Observe failed builtin-pool calls without replacing LVGL or its allocator.
 * Successful calls return directly; zero-size requests are not failures.
 * caps=0 in the event stream explicitly denotes the separate LVGL pool. */
extern void *__real_lv_malloc_core(size_t bytes);
extern void *__real_lv_realloc_core(void *p, size_t bytes);
void *__wrap_lv_malloc_core(size_t bytes)
{
    void *p = __real_lv_malloc_core(bytes);
    if (!p && bytes) panel_render_diag_alloc_failure(bytes, 0U);
    return p;
}
void *__wrap_lv_realloc_core(void *p, size_t bytes)
{
    void *result = __real_lv_realloc_core(p, bytes);
    if (!result && bytes) panel_render_diag_alloc_failure(bytes, 0U);
    return result;
}
