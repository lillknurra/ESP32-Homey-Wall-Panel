#include "athom_oauth_runtime.h"
#ifdef ESP_PLATFORM
#include "athom_auth_store.h"
#include "athom_cloud_client.h"
#include "athom_oauth_config.h"
#include "athom_oauth_flow.h"
#include "phone_provisioning.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mdns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "athom_oauth";
static bool s_mdns_started;
static athom_oauth_session_t s_session;
static athom_oauth_result_t s_last = ATHOM_OAUTH_ERR_STATE;
static athom_cloud_state_t s_cloud;
static char s_code[ATHOM_OAUTH_CODE_MAX];
static bool s_worker_running;
static bool s_select_worker_running;
static bool s_restore_worker_running;
static bool s_restore_started;
static bool s_schema_refresh_running;
static QueueHandle_t s_homey_command_queue;
static TaskHandle_t s_homey_command_worker_task;

typedef enum {
    ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA = 1,
} athom_homey_command_t;
static const char *s_state_name = "idle";
static uint32_t s_runtime_id;
static uint32_t s_select_attempt;

static int64_t now_s(void){return esp_timer_get_time()/1000000LL;}
static void zero_secure(void *p,size_t n){volatile unsigned char*q=p;while(n--)*q++=0U;}
static bool unreserved(unsigned char c){return(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='.'||c=='_'||c=='~';}

static bool pct(const char*in,char*out,size_t cap)
{
    static const char h[]="0123456789ABCDEF";
    size_t o=0;
    for(size_t i=0;in&&in[i];i++){
        unsigned char c=(unsigned char)in[i];
        if(unreserved(c)){if(o+1>=cap)return false;out[o++]=(char)c;}
        else{if(o+3>=cap)return false;out[o++]='%';out[o++]=h[c>>4];out[o++]=h[c&15];}
    }
    if(o>=cap)return false;
    out[o]=0;
    return true;
}

esp_err_t athom_oauth_runtime_get_selected_homey_id(char *out, size_t capacity)
{
    if (out == NULL || capacity == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    out[0] = '\0';
    if (s_cloud.selected_homey.id[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    if (strlcpy(out, s_cloud.selected_homey.id, capacity) >= capacity) {
        out[0] = '\0';
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t athom_oauth_runtime_on_wifi_online(void)
{
    if(s_mdns_started)return ESP_OK;
    esp_err_t e=mdns_init();
    if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)return e;
    e=mdns_hostname_set("homey-panel");
    if(e!=ESP_OK)return e;
    (void)mdns_instance_name_set("ESP32 Homey Wall Panel");
    e=mdns_service_add(NULL,"_http","_tcp",80,NULL,0);
    if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)return e;
    s_mdns_started=true;
    ESP_LOGI(TAG,"mDNS ready hostname=homey-panel.local");
    return ESP_OK;
}

static void publish_cloud_state(void)
{
    athom_auth_record_t *record = calloc(1U, sizeof(*record));
    if (record == NULL) {
        ESP_LOGE(TAG, "Homey auth persistence allocation failed");
        return;
    }

    record->tokens = s_cloud.tokens;
    record->selected_homey = s_cloud.selected_homey;
    memcpy(record->homey_session_token, s_cloud.homey_session_token,
           sizeof(record->homey_session_token));
    record->expires_at_s = s_cloud.expires_at_s;
    record->zone_count = s_cloud.zone_count;
    record->device_count = s_cloud.device_count;

    esp_err_t err = athom_auth_store_publish(record);
    zero_secure(record, sizeof(*record));
    free(record);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Homey auth persistence failed: %s",
                 esp_err_to_name(err));
    }
}

static void oauth_worker(void *arg)
{
    (void)arg;
    s_state_name = "token_exchange";
    esp_err_t err = athom_cloud_exchange_code(s_code, &s_cloud);
    zero_secure(s_code, sizeof(s_code));
    if (err == ESP_OK) {
        s_state_name = "fetching_homeys";
        err = athom_cloud_fetch_user_homeys(&s_cloud);
    }
    if (err == ESP_OK) {
        s_state_name = "homey_selection_required";
        publish_cloud_state();
        ESP_LOGI(TAG, "Athom OAuth complete homey_count=%u",
                 (unsigned)s_cloud.homeys.count);
    } else {
        s_state_name = "oauth_error";
        ESP_LOGE(TAG, "Athom OAuth failed: %s", esp_err_to_name(err));
    }
    s_worker_running = false;
    vTaskDelete(NULL);
}

static esp_err_t client_config_post(httpd_req_t*r){athom_oauth_client_config_t cur;bool present=false;esp_err_t e=athom_oauth_client_config_load(&cur,&present);zero_secure(&cur,sizeof(cur));if(e!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"config read failed");if(present){httpd_resp_set_status(r,"409 Conflict");return httpd_resp_sendstr(r,"{\"stored\":false}");}if(r->content_len<=0||r->content_len>1024)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"bad size");char*b=calloc(1,(size_t)r->content_len+1);if(!b)return ESP_ERR_NO_MEM;int n=httpd_req_recv(r,b,r->content_len);if(n!=r->content_len){zero_secure(b,(size_t)r->content_len+1);free(b);return ESP_FAIL;}cJSON*j=cJSON_ParseWithLength(b,n);cJSON*id=j?cJSON_GetObjectItemCaseSensitive(j,"client_id"):NULL;cJSON*sec=j?cJSON_GetObjectItemCaseSensitive(j,"client_secret"):NULL;cJSON*uri=j?cJSON_GetObjectItemCaseSensitive(j,"redirect_uri"):NULL;athom_oauth_client_config_t cfg;memset(&cfg,0,sizeof(cfg));bool ok=cJSON_IsString(id)&&cJSON_IsString(sec)&&cJSON_IsString(uri)&&athom_oauth_client_config_prepare(&cfg,id->valuestring,sec->valuestring,uri->valuestring);if(j)cJSON_Delete(j);zero_secure(b,(size_t)r->content_len+1);free(b);if(!ok){zero_secure(&cfg,sizeof(cfg));return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"invalid config");}e=athom_oauth_client_config_save(&cfg);zero_secure(&cfg,sizeof(cfg));if(e==ESP_ERR_INVALID_STATE){httpd_resp_set_status(r,"409 Conflict");return httpd_resp_sendstr(r,"{\"stored\":false}");}if(e!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"save failed");ESP_LOGI(TAG,"Athom OAuth client configuration stored");s_state_name="login_required";httpd_resp_set_status(r,"201 Created");httpd_resp_set_type(r,"application/json");return httpd_resp_sendstr(r,"{\"stored\":true}");}

static esp_err_t login_get(httpd_req_t *r)
{
    athom_oauth_client_config_t *config =
        calloc(1U, sizeof(*config));
    char *encoded_client_id =
        calloc(1U, ATHOM_CLIENT_ID_MAX * 3U);
    char *encoded_redirect =
        calloc(1U, ATHOM_REDIRECT_URI_MAX * 3U);
    char *url = calloc(1U, 768U);

    if (config == NULL ||
        encoded_client_id == NULL ||
        encoded_redirect == NULL ||
        url == NULL) {
        if (config != NULL) {
            zero_secure(config, sizeof(*config));
            free(config);
        }

        if (encoded_client_id != NULL) {
            zero_secure(
                encoded_client_id,
                ATHOM_CLIENT_ID_MAX * 3U);
            free(encoded_client_id);
        }

        if (encoded_redirect != NULL) {
            zero_secure(
                encoded_redirect,
                ATHOM_REDIRECT_URI_MAX * 3U);
            free(encoded_redirect);
        }

        if (url != NULL) {
            zero_secure(url, 768U);
            free(url);
        }

        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Minnesallokering misslyckades");
    }

    bool present = false;
    esp_err_t err =
        athom_oauth_client_config_load(config, &present);

    if (err != ESP_OK || !present) {
        httpd_resp_set_status(r, "503 Service Unavailable");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");

        err = httpd_resp_sendstr(
            r,
            "Homey OAuth-klienten är inte provisionerad");

        zero_secure(config, sizeof(*config));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(config);
        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return err;
    }

    uint8_t raw[ATHOM_OAUTH_STATE_BYTES];
    char state[ATHOM_OAUTH_STATE_TEXT_MAX];

    esp_fill_random(raw, sizeof(raw));

    if (athom_oauth_session_begin(
            &s_session,
            now_s(),
            raw) != ATHOM_OAUTH_OK) {
        zero_secure(raw, sizeof(raw));
        zero_secure(state, sizeof(state));
        zero_secure(config, sizeof(*config));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(config);
        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return ESP_FAIL;
    }

    bool ok =
        athom_oauth_state_encode(raw, state) &&
        pct(
            config->client_id,
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U) &&
        pct(
            config->redirect_uri,
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);

    zero_secure(raw, sizeof(raw));
    zero_secure(config, sizeof(*config));
    free(config);

    if (!ok) {
        zero_secure(state, sizeof(state));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return ESP_ERR_INVALID_SIZE;
    }

    int written = snprintf(
        url,
        768U,
        "https://api.athom.com/oauth2/authorise?"
        "response_type=code&client_id=%s&redirect_uri=%s&state=%s",
        encoded_client_id,
        encoded_redirect,
        state);

    zero_secure(state, sizeof(state));
    zero_secure(
        encoded_client_id,
        ATHOM_CLIENT_ID_MAX * 3U);
    zero_secure(
        encoded_redirect,
        ATHOM_REDIRECT_URI_MAX * 3U);

    free(encoded_client_id);
    free(encoded_redirect);

    if (written <= 0 || written >= 768) {
        zero_secure(url, 768U);
        free(url);
        return ESP_ERR_INVALID_SIZE;
    }

    s_state_name = "awaiting_callback";

    httpd_resp_set_status(r, "302 Found");
    httpd_resp_set_hdr(r, "Location", url);

    err = httpd_resp_send(r, NULL, 0);

    zero_secure(url, 768U);
    free(url);

    return err;
}

static esp_err_t callback_get(httpd_req_t*r)
{
    char q[1024]={0};
    char state_text[ATHOM_OAUTH_STATE_TEXT_MAX]={0};
    char code[ATHOM_OAUTH_CODE_MAX]={0};
    size_t n=httpd_req_get_url_query_len(r);
    if(n==0||n>=sizeof(q)){
        s_last=ATHOM_OAUTH_ERR_ARGUMENT;
        return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Callback saknar parametrar");
    }
    if(httpd_req_get_url_query_str(r,q,sizeof(q))!=ESP_OK)return ESP_FAIL;
    bool has_state=httpd_query_key_value(q,"state",state_text,sizeof(state_text))==ESP_OK;
    bool has_code=httpd_query_key_value(q,"code",code,sizeof(code))==ESP_OK;
    s_last=athom_oauth_callback_consume(&s_session,now_s(),has_state?state_text:NULL,has_code);
    zero_secure(q,sizeof(q));zero_secure(state_text,sizeof(state_text));
    if(s_last!=ATHOM_OAUTH_OK){
        zero_secure(code,sizeof(code));
        return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Homey-inloggningen kunde inte verifieras");
    }
    if (s_worker_running || s_restore_worker_running) {
        zero_secure(code, sizeof(code));
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "Homey-inloggning pågår redan");
    }
    zero_secure(s_code,sizeof(s_code));
    memcpy(s_code,code,strlen(code)+1U);
    zero_secure(code,sizeof(code));
    s_worker_running=true;
    if (xTaskCreate(oauth_worker,"athom_oauth",12288,NULL,5,NULL)!=pdPASS) {
        s_worker_running=false;zero_secure(s_code,sizeof(s_code));
        return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Kunde inte starta Homey-inloggningen");
    }
    httpd_resp_set_type(r,"text/html; charset=utf-8");
    return httpd_resp_sendstr(r,
        "<!doctype html><html lang=sv><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Homey</title><body><h1>Inloggningen behandlas</h1>"
        "<p>Återgå till Homey Panel. Dina Homeys hämtas nu.</p></body></html>");
}

static esp_err_t status_get(httpd_req_t *r)
{
    char *body = calloc(1U, 4096U);
    if (body == NULL) return ESP_ERR_NO_MEM;
    const athom_homey_t *selected =
        s_cloud.selected_homey.id[0] != '\0' ? &s_cloud.selected_homey : NULL;
    bool ok = athom_homey_status_json(
        body, 4096U, s_state_name, &s_cloud.homeys, selected,
        s_cloud.zone_count, s_cloud.device_count);
    if (!ok) {
        free(body);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t body_length = strlen(body);

    if (body_length == 0U ||
        body[body_length - 1U] != 125) {
        zero_secure(body, 4096U);
        free(body);
        return ESP_ERR_INVALID_RESPONSE;
    }

    int appended = snprintf(
        body + body_length - 1U,
        4096U - body_length + 1U,
        ",\"detail\":\"%s\","
        "\"last_error\":%d,"
        "\"http_status\":%d,"
        "\"runtime_id\":%u,"
        "\"select_attempt\":%u}",
        athom_cloud_diagnostic_stage(),
        (int)athom_cloud_diagnostic_error(),
        athom_cloud_diagnostic_http_status(),
        (unsigned)s_runtime_id,
        (unsigned)s_select_attempt);

    if (appended <= 0 ||
        (size_t)appended >= 4096U - body_length + 1U) {
        zero_secure(body, 4096U);
        free(body);
        return ESP_ERR_INVALID_SIZE;
    }

    httpd_resp_set_type(r,"application/json; charset=utf-8");
    esp_err_t err=httpd_resp_sendstr(r,body);
    zero_secure(body,4096U);free(body);return err;
}

typedef struct {
    char homey_id[ATHOM_HOMEY_ID_MAX];
} athom_select_work_t;

static esp_err_t connect_and_fetch_inventory(const char *homey_id)
{
    char selected_homey_id[ATHOM_HOMEY_ID_MAX] = {0};
    size_t selected_homey_id_length = strnlen(homey_id, sizeof(selected_homey_id));
    if (selected_homey_id_length == 0U || selected_homey_id_length >= sizeof(selected_homey_id)) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(selected_homey_id, homey_id, selected_homey_id_length + 1U);

    ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_begin attempt=1");
    esp_err_t err = athom_cloud_fetch_user_homeys(&s_cloud);
    const int first_homeys_http_status = athom_cloud_diagnostic_http_status();
    ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_end attempt=1 http_status=%d result=%s error=%s",
             first_homeys_http_status,
             err == ESP_OK ? "success" : "failure",
             esp_err_to_name(err));

    if (first_homeys_http_status == 401) {
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=token_refresh_begin");
        err = athom_cloud_refresh(&s_cloud);
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=token_refresh_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_begin attempt=2");
            err = athom_cloud_fetch_user_homeys(&s_cloud);
            const int second_homeys_http_status = athom_cloud_diagnostic_http_status();
            ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_end attempt=2 http_status=%d result=%s error=%s",
                     second_homeys_http_status,
                     err == ESP_OK ? "success" : "failure",
                     esp_err_to_name(err));
        }
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=connect_begin");
        err = athom_cloud_select_and_connect(&s_cloud, selected_homey_id);
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=connect_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=inventory_begin");
        err = athom_cloud_fetch_inventory(&s_cloud);
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=inventory_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
    }
    zero_secure(selected_homey_id, sizeof(selected_homey_id));
    return err;
}

static void select_worker(void *arg)
{
    athom_select_work_t *work = (athom_select_work_t *)arg;

    /*
     * Give the HTTP task time to transmit its 202 response before the
     * network-heavy Homey connection work starts.
     */
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_err_t err = connect_and_fetch_inventory(work->homey_id);

    zero_secure(work, sizeof(*work));
    free(work);

    if (err == ESP_OK) {
        s_state_name = "ready";
        publish_cloud_state();

        phone_provisioning_show_live_ready(
            s_cloud.selected_homey.name);

        ESP_LOGI(
            TAG,
            "Homey connection complete zones=%u devices=%u",
            (unsigned)s_cloud.zone_count,
            (unsigned)s_cloud.device_count);
    } else {
        s_state_name = "homey_connection_error";

        ESP_LOGE(
            TAG,
            "Homey connection failed: %s",
            esp_err_to_name(err));
    }

    s_select_worker_running = false;
    vTaskDelete(NULL);
}

static esp_err_t select_post(httpd_req_t *r)
{
    char body[256] = {0};

    if (r->content_len <= 0 ||
        r->content_len >= (int)sizeof(body)) {
        return httpd_resp_send_err(
            r,
            HTTPD_400_BAD_REQUEST,
            "homey_id saknas");
    }

    int received = httpd_req_recv(r, body, r->content_len);

    if (received != r->content_len) {
        zero_secure(body, sizeof(body));
        return ESP_FAIL;
    }

    body[received] = 0;

    athom_select_work_t *work = calloc(1U, sizeof(*work));

    if (work == NULL) {
        zero_secure(body, sizeof(body));
        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Minnesallokering misslyckades");
    }

    esp_err_t parse_err = httpd_query_key_value(
        body,
        "homey_id",
        work->homey_id,
        sizeof(work->homey_id));

    zero_secure(body, sizeof(body));

    if (parse_err != ESP_OK || work->homey_id[0] == 0) {
        zero_secure(work, sizeof(*work));
        free(work);

        return httpd_resp_send_err(
            r,
            HTTPD_400_BAD_REQUEST,
            "homey_id saknas");
    }

    if (s_worker_running ||
        s_select_worker_running ||
        s_restore_worker_running) {
        zero_secure(work, sizeof(*work));
        free(work);

        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");

        return httpd_resp_sendstr(
            r,
            "Ett Homey-jobb pågår redan");
    }

    s_select_worker_running = true;
    s_select_attempt++;
    s_state_name = "connecting_homey";

    if (xTaskCreate(
            select_worker,
            "athom_select",
            24576,
            work,
            5,
            NULL) != pdPASS) {
        s_select_worker_running = false;
        s_state_name = "homey_selection_required";

        zero_secure(work, sizeof(*work));
        free(work);

        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Kunde inte starta Homey-anslutningen");
    }

    httpd_resp_set_status(r, "202 Accepted");
    httpd_resp_set_type(r, "application/json; charset=utf-8");
    httpd_resp_set_hdr(r, "Cache-Control", "no-store");

    return httpd_resp_sendstr(
        r,
        "{\"accepted\":true,\"state\":\"connecting_homey\"}");
}

static void homey_command_worker(void *arg)
{
    (void)arg;
    athom_homey_command_t command;

    for (;;) {
        if (xQueueReceive(s_homey_command_queue, &command, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (command != ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA) {
            continue;
        }

        s_schema_refresh_running = true;
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=begin");

        char selected_homey_id[ATHOM_HOMEY_ID_MAX] = {0};
        memcpy(selected_homey_id,
               s_cloud.selected_homey.id,
               sizeof(selected_homey_id));

        esp_err_t err = connect_and_fetch_inventory(selected_homey_id);
        zero_secure(selected_homey_id, sizeof(selected_homey_id));

        if (err == ESP_OK) {
            s_state_name = "ready";
            publish_cloud_state();
            phone_provisioning_show_live_ready(s_cloud.selected_homey.name);
        } else {
            s_state_name = "homey_connection_error";
        }

        ESP_LOGI(TAG,
                 "HOMEY_SCHEMA path=queued_refresh phase=end result=%s",
                 err == ESP_OK ? "success" : "failure");
        s_schema_refresh_running = false;
    }
}

static esp_err_t schema_refresh_get(httpd_req_t *r)
{
    httpd_resp_set_type(r, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(r, "Cache-Control", "no-store");

    if (!phone_provisioning_homey_runtime_ready() ||
        s_cloud.selected_homey.id[0] == 0 ||
        s_cloud.homey_session_token[0] == 0 ||
        s_homey_command_queue == NULL) {
        httpd_resp_set_status(r, "409 Conflict");
        return httpd_resp_sendstr(r, "not ready");
    }

    if (s_worker_running || s_select_worker_running ||
        s_restore_worker_running || s_schema_refresh_running ||
        uxQueueMessagesWaiting(s_homey_command_queue) != 0U) {
        httpd_resp_set_status(r, "409 Conflict");
        return httpd_resp_sendstr(r, "busy");
    }

    athom_homey_command_t command =
        ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA;

    if (xQueueSend(s_homey_command_queue, &command, 0) != pdTRUE) {
        httpd_resp_set_status(r, "503 Service Unavailable");
        return httpd_resp_sendstr(r, "queue failed");
    }

    httpd_resp_set_status(r, "202 Accepted");
    return httpd_resp_sendstr(r, "queued");
}

static esp_err_t refresh_post(httpd_req_t *r)
{
    (void)r;
    s_state_name="refreshing";
    esp_err_t err=athom_cloud_refresh(&s_cloud);
    if(err==ESP_OK)err=athom_cloud_fetch_user_homeys(&s_cloud);
    if(err!=ESP_OK){
        s_state_name="login_required";
        return httpd_resp_send_err(r,HTTPD_401_UNAUTHORIZED,"Homey-inloggning krävs igen");
    }
    s_state_name=s_cloud.selected_homey.id[0]?"ready":"homey_selection_required";
    publish_cloud_state();
    return httpd_resp_sendstr(r,"ok");
}

static void auth_restore_worker(void *arg)
{
    (void)arg;
    s_state_name = "restoring_session";

    athom_auth_record_t *restored = calloc(1U, sizeof(*restored));
    if (restored == NULL) {
        ESP_LOGE(TAG, "Homey auth restore allocation failed");
        s_state_name = "login_required";
        s_restore_worker_running = false;
        vTaskDelete(NULL);
        return;
    }

    bool present = false;
    esp_err_t restore_err = athom_auth_store_load(restored, &present);

    if (restore_err == ESP_OK && present) {
        memcpy(&s_cloud.tokens,
               &restored->tokens,
               sizeof(s_cloud.tokens));
        memcpy(&s_cloud.selected_homey,
               &restored->selected_homey,
               sizeof(s_cloud.selected_homey));
        memcpy(s_cloud.homey_session_token,
               restored->homey_session_token,
               sizeof(s_cloud.homey_session_token));

        s_cloud.expires_at_s = restored->expires_at_s;
        s_cloud.zone_count = restored->zone_count;
        s_cloud.device_count = restored->device_count;

        s_state_name = s_cloud.selected_homey.id[0] != '\0'
            ? "ready"
            : "login_required";

        if (s_cloud.selected_homey.name[0] != 0) {
            phone_provisioning_show_live_ready(
                s_cloud.selected_homey.name);
        }

        ESP_LOGI(TAG,
                 "Homey auth restore complete selected=%s zones=%u devices=%u",
                 s_cloud.selected_homey.id[0] != '\0' ? "true" : "false",
                 (unsigned)s_cloud.zone_count,
                 (unsigned)s_cloud.device_count);
    } else if (restore_err == ESP_OK) {
        s_state_name = "login_required";
        ESP_LOGI(TAG, "No stored Homey auth session");
    } else {
        s_state_name = "login_required";
        ESP_LOGW(TAG,
                 "Homey auth restore failed: %s",
                 esp_err_to_name(restore_err));
    }

    zero_secure(restored, sizeof(*restored));
    free(restored);
    s_restore_worker_running = false;
    vTaskDelete(NULL);
}

esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t s)
{
    if(!s)return ESP_ERR_INVALID_ARG;

    if (s_runtime_id == 0U) {
        s_runtime_id = esp_random();

        if (s_runtime_id == 0U) {
            s_runtime_id = 1U;
        }
    }

    const httpd_uri_t handlers[]={
        {"/homey/client-config",HTTP_POST,client_config_post,NULL},
        {"/homey/login",HTTP_GET,login_get,NULL},
        {"/oauth/callback",HTTP_GET,callback_get,NULL},
        {"/homey/live-status",HTTP_GET,status_get,NULL},
        {"/homey/live-select",HTTP_POST,select_post,NULL},
        {"/homey/live-refresh",HTTP_POST,refresh_post,NULL},
        {"/homey/debug/refresh-inventory-schema",HTTP_GET,schema_refresh_get,NULL}
    };
    for(size_t i=0;i<sizeof(handlers)/sizeof(handlers[0]);i++){
        esp_err_t e=httpd_register_uri_handler(s,&handlers[i]);
        if(e!=ESP_OK&&e!=ESP_ERR_HTTPD_HANDLER_EXISTS)return e;
    }

    if (s_homey_command_queue == NULL) {
        s_homey_command_queue = xQueueCreate(1U, sizeof(athom_homey_command_t));
        if (s_homey_command_queue == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_homey_command_worker_task == NULL) {
        if (xTaskCreate(
                homey_command_worker,
                "athom_command",
                24576,
                NULL,
                5,
                &s_homey_command_worker_task) != pdPASS) {
            vQueueDelete(s_homey_command_queue);
            s_homey_command_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_restore_started == false) {
        s_restore_started = true;
        s_restore_worker_running = true;
        s_state_name = "restoring_session";

        if (xTaskCreate(
                auth_restore_worker,
                "athom_restore",
                16384,
                NULL,
                5,
                NULL) != pdPASS) {
            s_restore_worker_running = false;
            s_restore_started = false;
            s_state_name = "login_required";

            ESP_LOGE(
                TAG,
                "Could not start Homey auth restore task");

            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}
#endif
