#include "athom_auth.h"
#include "athom_homey_client.h"
#include "athom_provisioning.h"
#include "athom_redaction.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    athom_credentials_t value;
    bool present;
    int save_count;
    int wipe_count;
} memory_store_t;

static athom_status_t mem_load(athom_credential_store_t *store, athom_credentials_t *out) {
    memory_store_t *m = store->context;
    if (!m->present) return ATHOM_ERR_STORAGE;
    *out = m->value;
    return ATHOM_OK;
}
static athom_status_t mem_save(athom_credential_store_t *store, const athom_credentials_t *value) {
    memory_store_t *m = store->context;
    m->value = *value;
    m->present = true;
    m->save_count++;
    return ATHOM_OK;
}
static athom_status_t mem_wipe(athom_credential_store_t *store) {
    memory_store_t *m = store->context;
    athom_secure_zero(&m->value, sizeof(m->value));
    m->present = false;
    m->wipe_count++;
    return ATHOM_OK;
}
static const athom_credential_store_vtable_t STORE_VTABLE = {
    .load = mem_load, .save = mem_save, .wipe = mem_wipe
};

typedef struct {
    int exchange_count;
    int refresh_count;
    int list_count;
    int inventory_count;
    bool fail_refresh;
    char observed_code[64];
} mock_transport_t;

static athom_status_t mock_exchange(
    athom_http_transport_t *transport,
    const athom_credentials_t *client,
    const char *code,
    athom_credentials_t *updated) {
    mock_transport_t *m = transport->context;
    assert(strcmp(client->client_id, "client-id") == 0);
    snprintf(m->observed_code, sizeof(m->observed_code), "%s", code);
    snprintf(updated->access_token, sizeof(updated->access_token), "%s", "access-secret");
    snprintf(updated->refresh_token, sizeof(updated->refresh_token), "%s", "refresh-secret");
    updated->expires_at_epoch_s = 2000;
    m->exchange_count++;
    return ATHOM_OK;
}
static athom_status_t mock_refresh(
    athom_http_transport_t *transport,
    const athom_credentials_t *current,
    athom_credentials_t *updated) {
    mock_transport_t *m = transport->context;
    assert(current->refresh_token[0] != '\0');
    m->refresh_count++;
    if (m->fail_refresh) return ATHOM_ERR_TRANSPORT;
    snprintf(updated->access_token, sizeof(updated->access_token), "%s", "new-access-secret");
    updated->expires_at_epoch_s = 4000;
    return ATHOM_OK;
}
static athom_status_t mock_list(
    athom_http_transport_t *transport,
    const char *access_token,
    athom_homey_list_t *out) {
    mock_transport_t *m = transport->context;
    assert(access_token[0] != '\0');
    out->count = 2;
    snprintf(out->items[0].id, sizeof(out->items[0].id), "%s", "homey-a");
    snprintf(out->items[1].id, sizeof(out->items[1].id), "%s", "homey-b");
    m->list_count++;
    return ATHOM_OK;
}
static athom_status_t mock_inventory(
    athom_http_transport_t *transport,
    const char *access_token,
    const char *homey_id,
    athom_discovery_strategy_t strategy,
    athom_inventory_summary_t *out) {
    mock_transport_t *m = transport->context;
    assert(strcmp(access_token, "access-secret") == 0 || strcmp(access_token, "new-access-secret") == 0);
    assert(strcmp(homey_id, "homey-b") == 0);
    assert(strategy == ATHOM_DISCOVERY_CLOUD);
    *out = (athom_inventory_summary_t){1,2,3,4,5};
    m->inventory_count++;
    return ATHOM_OK;
}
static const athom_http_transport_vtable_t HTTP_VTABLE = {
    .exchange_authorization_code = mock_exchange,
    .refresh_access_token = mock_refresh,
    .list_homeys = mock_list,
    .read_inventory = mock_inventory,
};

int main(void) {
    memory_store_t memory = {0};
    athom_credential_store_t store = { .vtable = &STORE_VTABLE, .context = &memory };
    mock_transport_t mock = {0};
    athom_http_transport_t transport = { .vtable = &HTTP_VTABLE, .context = &mock };
    athom_auth_context_t auth = { .store = &store, .transport = &transport };
    athom_provisioning_t portal = { .auth = &auth, .state = ATHOM_PROVISIONING_IDLE };

    char state[ATHOM_MAX_OAUTH_STATE] = {0};
    assert(athom_provisioning_start(&portal, "client-id", "client-secret",
        "https://callback.invalid", 1000, state, sizeof(state)) == ATHOM_OK);
    assert(state[0] != '\0');
    assert(portal.state == ATHOM_PROVISIONING_WAITING_FOR_CODE);

    assert(athom_provisioning_submit_code(&portal, "wrong", "code", 1001)
        == ATHOM_ERR_STATE_MISMATCH);
    portal.state = ATHOM_PROVISIONING_WAITING_FOR_CODE;
    assert(athom_provisioning_submit_code(&portal, state, "authorization-code", 1001)
        == ATHOM_OK);
    assert(mock.exchange_count == 1);
    assert(strcmp(mock.observed_code, "authorization-code") == 0);

    athom_homey_list_t homeys = {0};
    assert(athom_homey_list(&auth, 1100, &homeys) == ATHOM_OK);
    assert(homeys.count == 2);
    assert(athom_provisioning_select_homey(&portal, "missing", &homeys, ATHOM_DISCOVERY_CLOUD)
        == ATHOM_ERR_HOMEY_NOT_FOUND);
    portal.state = ATHOM_PROVISIONING_AUTHORIZED;
    assert(athom_provisioning_select_homey(&portal, "homey-b", &homeys, ATHOM_DISCOVERY_CLOUD)
        == ATHOM_OK);

    athom_inventory_summary_t summary = {0};
    assert(athom_homey_read_inventory(&auth, 1200, &summary) == ATHOM_OK);
    assert(summary.devices == 2 && mock.inventory_count == 1);

    memory.value.expires_at_epoch_s = 1200;
    mock.fail_refresh = true;
    athom_credentials_t token = {0};
    assert(athom_auth_ensure_access_token(&auth, 1300, &token) == ATHOM_ERR_REFRESH_FAILED);

    assert(!athom_mutation_allowed("setCapabilityValue"));
    assert(!athom_text_contains_secret("safe status", "access-secret"));
    assert(athom_text_contains_secret("bad access-secret output", "access-secret"));

    assert(athom_provisioning_wipe(&portal) == ATHOM_OK);
    assert(!memory.present && memory.wipe_count == 1);

    puts("PASS: ESP32-native Athom Cloud host tests");
    return 0;
}
