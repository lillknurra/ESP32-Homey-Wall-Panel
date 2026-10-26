/* Patch116I: real cJSON parsing and pure policy via GCC/Clang ASan/UBSan. */
#include "panel_update_manifest.h"
#include "panel_update_check.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned g_assertions;
#define REQUIRE(x) do { g_assertions++; if (!(x)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static const char VALID[] =
 "{\"schema_version\":1,"
 "\"hardware_model\":\"waveshare-esp32-s3-touch-lcd-4b-n16r8\","
 "\"project\":\"esp32_homey_wall_panel\","
 "\"channel\":\"stable\",\"release_sequence\":120,\"version\":\"1.2.0\","
 "\"notes\":\"Bugfixes and improvements\","
 "\"image\":{\"size_bytes\":1749280,"
 "\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
 "\"url\":\"https://github.com/lillknurra/ESP32-Homey-Wall-Panel/releases/download/"
 "panel-v1.2.0/esp32_homey_wall_panel_waveshare-esp32-s3-touch-lcd-4b-n16r8_1.2.0_signed.bin\"}}";

static panel_update_manifest_t read_valid(void)
{
    panel_update_manifest_t m = {0};
    REQUIRE(panel_update_manifest_parse(VALID, strlen(VALID), &m));
    REQUIRE(panel_update_manifest_metadata_valid(&m));
    REQUIRE(m.schema_version == 1U);
    REQUIRE(m.release_sequence == 120U);
    REQUIRE(m.image_size_bytes == 1749280U);
    REQUIRE(m.image_sha256[0] == 0xaa && m.image_sha256[31] == 0xaa);
    return m;
}

static void test_policy(void)
{
    panel_update_manifest_t m = read_valid();
    panel_update_local_t l = {
        .hardware_model = PANEL_UPDATE_MODEL_ID,
        .project = PANEL_UPDATE_PROJECT_ID,
        .channel = "stable",
        .installed_release_sequence = 110,
        .installed_version = "1.1.0",
        .has_ota_partitions = true,
        .inactive_ota_capacity = 0x400000,
    };
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_NEWER_AVAILABLE);
    l.has_ota_partitions = false;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_NEWER_NEEDS_MIGRATION);
    l.has_ota_partitions = true;
    l.inactive_ota_capacity = m.image_size_bytes - 1U;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_NEWER_TOO_LARGE);
    l.inactive_ota_capacity = m.image_size_bytes;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_NEWER_AVAILABLE);
    l.installed_release_sequence = 120;
    l.installed_version = "1.2.0";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_CURRENT);
    l.installed_version = "1.2.1";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_REJECTED);
    l.installed_release_sequence = 121;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_CURRENT);
    l.installed_release_sequence = 0;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_UNVERSIONED);
    l.installed_release_sequence = 110;
    l.installed_version = "148ecda-dirty";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_UNVERSIONED);
    l.installed_version = "1.1.0";
    l.channel = "beta";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_REJECTED);
    l.channel = "stable";
    l.hardware_model = "other-panel";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_REJECTED);
    l.hardware_model = PANEL_UPDATE_MODEL_ID;
    l.project = "other-project";
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_REJECTED);
    l.project = PANEL_UPDATE_PROJECT_ID;
    REQUIRE(panel_update_manifest_evaluate(NULL, &l) == PANEL_UPDATE_REJECTED);
    REQUIRE(panel_update_manifest_evaluate(&m, NULL) == PANEL_UPDATE_REJECTED);
    m.release_sequence = 0;
    REQUIRE(panel_update_manifest_evaluate(&m, &l) == PANEL_UPDATE_REJECTED);
    m = read_valid();
    memset(m.image_sha256, 0, sizeof(m.image_sha256));
    REQUIRE(!panel_update_manifest_metadata_valid(&m));
    m = read_valid();
    m.image_size_bytes = PANEL_UPDATE_MAX_IMAGE_BYTES + 1U;
    REQUIRE(!panel_update_manifest_metadata_valid(&m));
    m = read_valid();
    strcpy(m.image_url, "http://evil.example/firmware.bin");
    REQUIRE(!panel_update_manifest_metadata_valid(&m));
    m = read_valid();
    memset(m.version, '9', sizeof(m.version));
    REQUIRE(!panel_update_manifest_metadata_valid(&m));
    m = read_valid();
    memset(m.notes, 'A', sizeof(m.notes));
    REQUIRE(!panel_update_manifest_metadata_valid(&m));
}

static void reject_payload(const char *body)
{
    panel_update_manifest_t m;
    memset(&m, 0xA5, sizeof(m));
    REQUIRE(!panel_update_manifest_parse(body, strlen(body), &m));
    for (size_t i = 0; i < sizeof(m); i++) REQUIRE(((uint8_t *)&m)[i] == 0xA5);
}

static void test_parser_errors(void)
{
    reject_payload("{}");
    reject_payload("[]");
    reject_payload("null");
    reject_payload("true");
    reject_payload("{\"schema_version\":1}");
    reject_payload("{\"schema_version\":1,\"schema_version\":2}");
    reject_payload("{\"notes\":\"\\u0000evil\"}");
    reject_payload("{\"notes\":\"abc\"} {} ");
    panel_update_manifest_t m;
    char b[PANEL_UPDATE_MANIFEST_MAX_BYTES + 2U];
    memset(b, ' ', sizeof(b));
    REQUIRE(!panel_update_manifest_parse(b, sizeof(b), &m));
    REQUIRE(!panel_update_manifest_parse(NULL, 1, &m));
    REQUIRE(!panel_update_manifest_parse(VALID, strlen(VALID), NULL));
    char validnul[sizeof(VALID) + 8];
    memcpy(validnul, VALID, sizeof(VALID));
    REQUIRE(!panel_update_manifest_parse(validnul, sizeof(VALID), &m));

    struct mutation { const char *needle, *replacement; } substitutions[] = {
      {"\"schema_version\":1", "\"schema_version\":2"},
      {"\"schema_version\":1", "\"schema_version\":true"},
      {"\"schema_version\":1", "\"schema_version\":1.5"},
      {"\"release_sequence\":120", "\"release_sequence\":0"},
      {"\"release_sequence\":120", "\"release_sequence\":4294967296"},
      {"\"release_sequence\":120", "\"release_sequence\":120.5"},
      {"\"release_sequence\":120", "\"release_sequence\":null"},
      {"\"channel\":\"stable\"", "\"channel\":\"production\""},
      {"\"channel\":\"stable\"", "\"channel\":\"stable\",\"channel\":\"stable\""},
      {"\"version\":\"1.2.0\"", "\"version\":\"01.2.0\""},
      {"\"version\":\"1.2.0\"", "\"version\":\"148ecda-dirty\""},
      {"\"version\":\"1.2.0\"", "\"version\":\"1.2.0\",\"unknown\":2"},
      {"\"size_bytes\":1749280", "\"size_bytes\":0"},
      {"\"size_bytes\":1749280", "\"size_bytes\":4194305"},
      {"\"size_bytes\":1749280", "\"size_bytes\":true"},
      {"\"notes\":\"Bugfixes and improvements\"", "\"notes\":\"a\\u0000b\""},
      {"\"notes\":\"Bugfixes and improvements\"", "\"notes\":null"},
      {"\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"", "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\""},
      {"\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"", "\"sha256\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\""},
      {"\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"", "\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\",\"sha256\":\"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\""},
      {"https://github.com/lillknurra/ESP32-Homey-Wall-Panel", "https://evil.example"},
      {"panel-v1.2.0/", "panel-v1.2.1/"},
      {"1.2.0_signed.bin", "1.2.0_other.bin"},
      {"\"hardware_model\":\"waveshare-esp32-s3-touch-lcd-4b-n16r8\"", "\"hardware_model\":\"A bad model\""},
      {"\"project\":\"esp32_homey_wall_panel\"", "\"project\":\"other/project\""},
    };
    for (size_t i = 0; i < sizeof(substitutions)/sizeof(substitutions[0]); i++) {
        const char *needle = substitutions[i].needle;
        const char *found = strstr(VALID, needle);
        REQUIRE(found != NULL);
        size_t a = (size_t)(found - VALID), n = strlen(needle), r = strlen(substitutions[i].replacement);
        size_t tail = strlen(found + n);
        REQUIRE(a + r + tail + 1U < sizeof(b));
        memcpy(b, VALID, a);
        memcpy(b + a, substitutions[i].replacement, r);
        memcpy(b + a + r, found + n, tail + 1U);
        reject_payload(b);
    }
}

static void test_fuzz(void)
{
    /* Deterministic mutation: every flipped byte must not crash the decoder. */
    char copy[sizeof(VALID)];
    panel_update_manifest_t out;
    unsigned seed = 0x1161U;
    for (size_t trial = 0; trial < 18000U; trial++) {
        memcpy(copy, VALID, sizeof(VALID));
        seed = seed * 1664525U + 1013904223U;
        const size_t index = seed % (sizeof(VALID) - 1U);
        seed = seed * 1664525U + 1013904223U;
        copy[index] = (char)(seed & 0xffU);
        (void)panel_update_manifest_parse(copy, strlen(VALID), &out);
        g_assertions++;
    }
}

static void test_read_only_scheduler(void)
{
    panel_update_check_t state;
    panel_update_check_init(&state, 1000U, 15U);
    REQUIRE(state.next_due_ms == 91015U);
    REQUIRE(!state.fresh && !state.in_flight);
    uint32_t gen = 0;
    const panel_update_local_t local = {
        .hardware_model = PANEL_UPDATE_MODEL_ID,
        .project = PANEL_UPDATE_PROJECT_ID,
        .channel = "stable",
        .installed_release_sequence = 100,
        .installed_version = "1.0.0",
        .has_ota_partitions = true,
        .inactive_ota_capacity = 0x400000U,
    };
    REQUIRE(!panel_update_check_start(&state, 91000, true, true, false, &gen));
    REQUIRE(!panel_update_check_start(&state, 92000, false, true, false, &gen));
    REQUIRE(!panel_update_check_start(&state, 92000, true, false, false, &gen));
    REQUIRE(panel_update_check_start(&state, 92000, true, true, false, &gen));
    REQUIRE(gen == 1 && state.in_flight);
    REQUIRE(!panel_update_check_start(&state, 92001, true, true, true, &gen));
    REQUIRE(!panel_update_check_complete(&state, 2, 92001, true, VALID, strlen(VALID), &local));
    REQUIRE(!state.fresh && state.in_flight);
    REQUIRE(!panel_update_check_tick(&state, 121999));
    REQUIRE(panel_update_check_tick(&state, 122000));
    REQUIRE(!state.in_flight && !state.fresh);
    REQUIRE(!panel_update_check_complete(&state, 1, 122001, true, VALID, strlen(VALID), &local));
    REQUIRE(!panel_update_check_start(&state, 122002, true, true, false, &gen));
    REQUIRE(panel_update_check_start(&state, 122003, true, true, true, &gen));
    REQUIRE(gen == 2);
    REQUIRE(panel_update_check_complete(&state, gen, 122004, true, VALID, strlen(VALID), &local));
    REQUIRE(state.fresh && state.status == PANEL_UPDATE_NEWER_AVAILABLE);
    REQUIRE(!state.in_flight);
    REQUIRE(state.next_due_ms == 122004 + PANEL_UPDATE_PERIOD_MS);
    REQUIRE(!panel_update_check_complete(&state, gen, 122006, true, VALID, strlen(VALID), &local));
    REQUIRE(panel_update_check_start(&state, 122100, true, true, true, &gen));
    REQUIRE(panel_update_check_complete(&state, gen, 122101, false, VALID, strlen(VALID), &local));
    REQUIRE(!state.fresh && state.status == PANEL_UPDATE_REJECTED);
    REQUIRE(panel_update_check_start(&state, 123000, true, true, true, &gen));
    REQUIRE(panel_update_check_complete(&state, gen, 123001, true, "{}", 2, &local));
    REQUIRE(!state.fresh);
    REQUIRE(panel_update_check_start(&state, 123002, true, true, true, &gen));
    REQUIRE(panel_update_check_complete(&state, gen, 123001, true, VALID, strlen(VALID), &local));
    REQUIRE(!state.in_flight && !state.fresh);
    REQUIRE(panel_update_check_start(&state, 123004, true, true, true, &gen));
    REQUIRE(panel_update_check_complete(&state, gen, 123005, true, VALID, strlen(VALID), &local));
    REQUIRE(state.status == PANEL_UPDATE_NEWER_AVAILABLE && state.fresh);
    panel_update_check_init(&state, UINT64_MAX - 500U, 500000U);
    REQUIRE(state.next_due_ms == UINT64_MAX);
    REQUIRE(!panel_update_check_start(&state, UINT64_MAX - 1U, true, true, false, &gen));
    REQUIRE(panel_update_check_start(&state, UINT64_MAX - 1U, true, true, true, &gen));
    REQUIRE(panel_update_check_tick(&state, 0U));
    REQUIRE(!state.fresh && state.status == PANEL_UPDATE_REJECTED);
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        FILE *f = fopen(argv[1], "rb");
        REQUIRE(f != NULL);
        char buf[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
        size_t n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        panel_update_manifest_t m;
        REQUIRE(panel_update_manifest_parse(buf, n, &m));
        printf("VALID_MANIFEST version=%s sequence=%u\n", m.version, m.release_sequence);
        return 0;
    }
    test_policy();
    test_read_only_scheduler();
    test_parser_errors();
    test_fuzz();
    printf("PATCH116I_HOST_C_TESTS=PASS assertions=%u\n", g_assertions);
    return 0;
}
