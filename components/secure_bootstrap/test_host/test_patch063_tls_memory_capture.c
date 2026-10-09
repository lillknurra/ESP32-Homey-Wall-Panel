#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define MALLOC_CAP_INTERNAL 0x01U
#define MALLOC_CAP_8BIT 0x18U

typedef void (*heap_caps_failed_alloc_callback_t)(
    size_t requested_size, uint32_t caps, const char *function_name);

typedef struct {
    bool capture_attempted;
    bool hook_registered;
    uint32_t matching_failure_count;
    uint32_t first_requested_size;
    uint32_t last_requested_size;
    uint32_t max_requested_size;
    uint32_t failure_caps;
    bool all_heap_caps_calloc;
    uint32_t internal_8bit_free_before;
    uint32_t internal_8bit_largest_before;
    uint32_t internal_8bit_minimum_before;
    uint32_t internal_8bit_free_at_failure;
    uint32_t internal_8bit_largest_at_failure;
    uint32_t internal_8bit_minimum_at_failure;
    uint32_t internal_8bit_free_after;
    uint32_t internal_8bit_largest_after;
    uint32_t internal_8bit_minimum_after;
} athom_tls_memory_diagnostic_t;

static heap_caps_failed_alloc_callback_t s_callback;
static uint32_t s_free_bytes;
static uint32_t s_largest_bytes;
static uint32_t s_minimum_bytes;
static unsigned s_registration_count;

static esp_err_t heap_caps_register_failed_alloc_callback(
    heap_caps_failed_alloc_callback_t callback)
{
    s_registration_count++;
    s_callback = callback;
    return ESP_OK;
}

static size_t heap_caps_get_free_size(uint32_t caps)
{ assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)); return s_free_bytes; }
static size_t heap_caps_get_largest_free_block(uint32_t caps)
{ assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)); return s_largest_bytes; }
static size_t heap_caps_get_minimum_free_size(uint32_t caps)
{ assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)); return s_minimum_bytes; }

/* PATCH063_PRODUCTION_CAPTURE */

static void set_heap(uint32_t free_bytes, uint32_t largest, uint32_t minimum)
{
    s_free_bytes = free_bytes;
    s_largest_bytes = largest;
    s_minimum_bytes = minimum;
}

static void test_cloud_perform_window_captures_matching_internal_failure(void)
{
    set_heap(50000U, 30000U, 40000U);
    patch019a16e_begin_transport_alloc_capture();
    assert(s_registration_count == 1U && s_callback != NULL);
    assert(s_patch019a16e_memory_diagnostic.capture_attempted);
    assert(s_patch019a16e_memory_diagnostic.hook_registered);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_free_before == 50000U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_largest_before == 30000U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_minimum_before == 40000U);

    set_heap(12000U, 1024U, 10000U);
    s_callback(2048U, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, "heap_caps_calloc");
    s_callback(4096U, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, "heap_caps_malloc");
    s_callback(8192U, MALLOC_CAP_8BIT, "heap_caps_calloc");
    assert(s_patch019a16e_memory_diagnostic.matching_failure_count == 2U);
    assert(s_patch019a16e_memory_diagnostic.first_requested_size == 2048U);
    assert(s_patch019a16e_memory_diagnostic.last_requested_size == 4096U);
    assert(s_patch019a16e_memory_diagnostic.max_requested_size == 4096U);
    assert(s_patch019a16e_memory_diagnostic.failure_caps ==
           (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    assert(!s_patch019a16e_memory_diagnostic.all_heap_caps_calloc);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_free_at_failure == 12000U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_largest_at_failure == 1024U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_minimum_at_failure == 10000U);

    set_heap(49000U, 29000U, 40000U);
    patch019a16e_finish_transport_alloc_capture();
    assert(!s_patch019a16e_transport_capture_active);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_free_after == 49000U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_largest_after == 29000U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_minimum_after == 40000U);
    s_callback(1024U, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, "heap_caps_calloc");
    assert(s_patch019a16e_memory_diagnostic.matching_failure_count == 2U);
}

static void test_next_attempt_clears_failure_and_retains_heap_samples(void)
{
    set_heap(48000U, 28000U, 39000U);
    patch019a16e_begin_transport_alloc_capture();
    assert(s_registration_count == 1U);
    assert(s_patch019a16e_memory_diagnostic.matching_failure_count == 0U);
    assert(s_patch019a16e_memory_diagnostic.first_requested_size == 0U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_free_at_failure == 0U);
    assert(s_patch019a16e_memory_diagnostic.internal_8bit_free_before == 48000U);
    patch019a16e_finish_transport_alloc_capture();
}

int main(void)
{
    test_cloud_perform_window_captures_matching_internal_failure();
    test_next_attempt_clears_failure_and_retains_heap_samples();
    puts("PATCH063_TLS_MEMORY_CAPTURE_TESTS PASS");
    return 0;
}
