#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATHOM_TOKEN_MAX 4096U
#define ATHOM_HOMEY_MAX 8U
#define ATHOM_HOMEY_ID_MAX 96U
#define ATHOM_HOMEY_NAME_MAX 128U
#define ATHOM_HOMEY_PLATFORM_MAX 16U
#define ATHOM_HOMEY_URL_MAX 320U

typedef struct {
    char access_token[ATHOM_TOKEN_MAX];
    char refresh_token[ATHOM_TOKEN_MAX];
    uint32_t expires_in_s;
} athom_token_set_t;

typedef struct {
    char id[ATHOM_HOMEY_ID_MAX];
    char name[ATHOM_HOMEY_NAME_MAX];
    char platform[ATHOM_HOMEY_PLATFORM_MAX];
    char local_url_secure[ATHOM_HOMEY_URL_MAX];
    char local_url[ATHOM_HOMEY_URL_MAX];
    char remote_url[ATHOM_HOMEY_URL_MAX];
} athom_homey_t;

typedef struct {
    athom_homey_t items[ATHOM_HOMEY_MAX];
    size_t count;
} athom_homey_list_t;

bool athom_token_set_apply_refresh(
    athom_token_set_t *current,
    const char *new_access_token,
    const char *new_refresh_token,
    uint32_t expires_in_s);

const char *athom_homey_preferred_url(const athom_homey_t *homey);

const athom_homey_t *athom_homey_find_exact(
    const athom_homey_list_t *list,
    const char *homey_id);

bool athom_homey_status_json(
    char *out,
    size_t capacity,
    const char *state,
    const athom_homey_list_t *homeys,
    const athom_homey_t *selected,
    size_t zone_count,
    size_t device_count);
