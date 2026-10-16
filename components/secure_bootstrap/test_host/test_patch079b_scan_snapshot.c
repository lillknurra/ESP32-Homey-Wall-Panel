#include "secure_bootstrap.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static secure_bootstrap_network_snapshot_t snapshot;
static pthread_mutex_t snapshot_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned iterations = 50000U;

static void *producer(void *unused)
{
    (void)unused;
    for (unsigned generation = 1U; generation <= iterations; ++generation) {
        secure_bootstrap_network_t values[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
        for (size_t i = 0; i < SECURE_BOOTSTRAP_MAX_NETWORKS; ++i) {
            snprintf(values[i].ssid, sizeof(values[i].ssid), "g%u-n%u", generation, (unsigned)i);
            values[i].rssi = (int8_t)(generation % 100U);
            values[i].security = SECURE_BOOTSTRAP_WIFI_SECURITY_WPA2;
        }
        assert(pthread_mutex_lock(&snapshot_mutex) == 0);
        assert(secure_bootstrap_network_snapshot_publish(
            &snapshot, values, SECURE_BOOTSTRAP_MAX_NETWORKS, 1U));
        assert(pthread_mutex_unlock(&snapshot_mutex) == 0);
    }
    return NULL;
}

static void *consumer(void *unused)
{
    (void)unused;
    for (unsigned attempt = 0U; attempt < iterations; ++attempt) {
        secure_bootstrap_network_t copy[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
        size_t count = 0U;
        assert(pthread_mutex_lock(&snapshot_mutex) == 0);
        const bool copied = secure_bootstrap_network_snapshot_copy(
            &snapshot, 1U, copy, SECURE_BOOTSTRAP_MAX_NETWORKS, &count);
        assert(pthread_mutex_unlock(&snapshot_mutex) == 0);
        if (!copied) continue;
        assert(count == SECURE_BOOTSTRAP_MAX_NETWORKS);
        const int8_t expected_rssi = copy[0].rssi;
        char prefix[16];
        const char *dash = strchr(copy[0].ssid, '-');
        assert(dash != NULL);
        size_t prefix_len = (size_t)(dash - copy[0].ssid);
        assert(prefix_len < sizeof(prefix));
        memcpy(prefix, copy[0].ssid, prefix_len);
        prefix[prefix_len] = '\0';
        for (size_t i = 0; i < count; ++i) {
            char expected[SECURE_BOOTSTRAP_SSID_MAX_LEN + 1U];
            snprintf(expected, sizeof(expected), "%s-n%u", prefix, (unsigned)i);
            assert(strcmp(copy[i].ssid, expected) == 0);
            assert(copy[i].rssi == expected_rssi);
            assert(copy[i].security == SECURE_BOOTSTRAP_WIFI_SECURITY_WPA2);
        }
    }
    secure_bootstrap_network_t stale[SECURE_BOOTSTRAP_MAX_NETWORKS];
    size_t stale_count = 99U;
    assert(pthread_mutex_lock(&snapshot_mutex) == 0);
    const bool stale_copy = secure_bootstrap_network_snapshot_copy(
        &snapshot, 2U, stale, SECURE_BOOTSTRAP_MAX_NETWORKS, &stale_count);
    assert(pthread_mutex_unlock(&snapshot_mutex) == 0);
    assert(!stale_copy && stale_count == 0U && stale[0].ssid[0] == '\0');
    return NULL;
}

int main(void)
{
    secure_bootstrap_scan_state_t scan = {0};
    assert(secure_bootstrap_scan_begin(&scan, 11U));
    secure_bootstrap_scan_cancel(&scan);
    assert(!secure_bootstrap_scan_begin(&scan, 12U));
    assert(!secure_bootstrap_scan_complete(&scan, 11U, 12U, true));
    assert(!scan.in_progress && scan.session_generation == 0U);
    assert(secure_bootstrap_scan_begin(&scan, 12U));
    assert(secure_bootstrap_scan_complete(&scan, 12U, 12U, true));
    assert(!scan.in_progress);

    pthread_t writer, reader;
    assert(pthread_create(&writer, NULL, producer, NULL) == 0);
    assert(pthread_create(&reader, NULL, consumer, NULL) == 0);
    assert(pthread_join(writer, NULL) == 0);
    assert(pthread_join(reader, NULL) == 0);
    puts("PASS: production scan cancel fence and snapshot copy/publish under concurrent producer and reader");
    return 0;
}
