#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATHOM_MAX_CLIENT_ID 96
#define ATHOM_MAX_CLIENT_SECRET 160
#define ATHOM_MAX_REDIRECT_URI 256
#define ATHOM_MAX_TOKEN 1024
#define ATHOM_MAX_HOMEY_ID 96
#define ATHOM_MAX_OAUTH_STATE 96
#define ATHOM_MAX_AUTH_CODE 768
#define ATHOM_MAX_HOMEYS 16

typedef enum {
    ATHOM_OK = 0,
    ATHOM_ERR_ARGUMENT,
    ATHOM_ERR_STATE_MISMATCH,
    ATHOM_ERR_STATE_EXPIRED,
    ATHOM_ERR_TOKEN_EXPIRED,
    ATHOM_ERR_REFRESH_FAILED,
    ATHOM_ERR_HOMEY_NOT_FOUND,
    ATHOM_ERR_NOT_PROVISIONED,
    ATHOM_ERR_STORAGE,
    ATHOM_ERR_TRANSPORT,
    ATHOM_ERR_RESPONSE,
    ATHOM_ERR_UNSUPPORTED,
} athom_status_t;

typedef enum {
    ATHOM_DISCOVERY_CLOUD = 0,
    ATHOM_DISCOVERY_REMOTE_FORWARDED = 1,
} athom_discovery_strategy_t;

typedef struct {
    char client_id[ATHOM_MAX_CLIENT_ID];
    char client_secret[ATHOM_MAX_CLIENT_SECRET];
    char redirect_uri[ATHOM_MAX_REDIRECT_URI];
    char access_token[ATHOM_MAX_TOKEN];
    char refresh_token[ATHOM_MAX_TOKEN];
    int64_t expires_at_epoch_s;
    char selected_homey_id[ATHOM_MAX_HOMEY_ID];
    athom_discovery_strategy_t strategy;
    bool provisioned;
} athom_credentials_t;

typedef struct {
    char id[ATHOM_MAX_HOMEY_ID];
    char display_name[128];
    bool online;
} athom_homey_summary_t;

typedef struct {
    athom_homey_summary_t items[ATHOM_MAX_HOMEYS];
    size_t count;
} athom_homey_list_t;

typedef struct {
    size_t zones;
    size_t devices;
    size_t flows;
    size_t advanced_flows;
    size_t moods;
} athom_inventory_summary_t;
