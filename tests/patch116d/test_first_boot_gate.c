#include "panel_ota_first_boot_gate.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "nvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static esp_partition_t partitions[] = {
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, 0x9000, 0x6000, "nvs" },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, 0xF000, 0x1000, "phy_init" },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, 0x10000, 0x600000, "factory" },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, 0x610000, 0x100000, "diaglog" },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, 0x710000, 0x2000, "otadata" },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, 0x720000, 0x400000, "ota_0" },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, 0xB20000, 0x400000, "ota_1" },
};
static const esp_partition_t *running;
static esp_ota_img_states_t state;
static esp_err_t state_result;
static esp_err_t desc_result;
static esp_err_t settings_result;
static esp_err_t confirm_result;
static esp_err_t invalid_result;
static bool fallback_possible;
static bool psram_available;
static size_t internal_largest;
static size_t psram_largest;
static bool hide_running;
static bool hide_partition;
static const char *desc_project;
static const char *desc_version;
static int confirm_count;
static int invalid_count;
static int open_count;
static int close_count;
static int read_only_mode_count;
static int state_read_count;
static int partition_read_count;
static int health_alloc_count;
static int tests;

static void reset_env(void) {
    partitions[0].address=0x9000;
    partitions[2].address=0x10000;
    partitions[3].address=0x610000;
    partitions[4].address=0x710000;
    partitions[5].address=0x720000;
    partitions[6].address=0xB20000;
    running=&partitions[5];
    state=ESP_OTA_IMG_PENDING_VERIFY;
    state_result=ESP_OK;
    desc_result=ESP_OK;
    settings_result=ESP_OK;
    confirm_result=ESP_OK;
    invalid_result=ESP_FAIL; /* fake reboot returns; gate must stop */
    fallback_possible=true;
    psram_available=true;
    internal_largest=16384;
    psram_largest=1024 * 1024;
    hide_running=false;
    hide_partition=false;
    desc_project="esp32_homey_wall_panel";
    desc_version="patch116d";
    confirm_count=invalid_count=open_count=close_count=read_only_mode_count=0;
    state_read_count=partition_read_count=health_alloc_count=0;
}

#define CHECK(msg, expected, c, inv) do { \
    ++tests; bool actual=panel_ota_first_boot_gate_after_nvs(); \
    if (actual!=(expected) || confirm_count!=(c) || invalid_count!=(inv)) { \
        fprintf(stderr,"FAIL: %s: gate=%d confirm=%d invalid=%d\n", \
                msg,(int)actual,confirm_count,invalid_count); exit(1); } \
} while(0)

const esp_partition_t *esp_partition_find_first(esp_partition_type_t type,
                                             esp_partition_subtype_t subtype,
                                             const char *label) {
    partition_read_count++;
    if (hide_partition) return NULL;
    for (size_t i=0;i<sizeof(partitions)/sizeof(partitions[0]);++i) {
        if (partitions[i].type==type && partitions[i].subtype==subtype &&
            strcmp(partitions[i].label,label)==0) return &partitions[i];
    }
    return NULL;
}
const esp_partition_t *esp_ota_get_running_partition(void) { return hide_running?NULL:running; }
esp_err_t esp_ota_get_state_partition(const esp_partition_t *p, esp_ota_img_states_t *out) {
    (void)p;state_read_count++; if (state_result==ESP_OK) *out=state;return state_result;
}
const esp_app_desc_t *esp_app_get_description(void) {
    static esp_app_desc_t desc;
    memset(&desc,0,sizeof(desc));
    strncpy(desc.project_name,"esp32_homey_wall_panel",sizeof(desc.project_name)-1);
    strncpy(desc.version,"patch116d",sizeof(desc.version)-1);
    return &desc;
}
esp_err_t esp_ota_get_partition_description(const esp_partition_t *p, esp_app_desc_t *out) {
    (void)p;if(desc_result!=ESP_OK)return desc_result;
    memset(out,0,sizeof(*out));
    strncpy(out->project_name,desc_project,sizeof(out->project_name)-1);
    strncpy(out->version,desc_version,sizeof(out->version)-1);
    return ESP_OK;
}
bool esp_ota_check_rollback_is_possible(void){ return fallback_possible; }
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void){confirm_count++;return confirm_result;}
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void){invalid_count++;return invalid_result;}
bool esp_psram_is_initialized(void){return psram_available;}
size_t heap_caps_get_largest_free_block(uint32_t caps){
    health_alloc_count++;
    return (caps&MALLOC_CAP_SPIRAM)?psram_largest:internal_largest;
}
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle){
    open_count++; if(mode==NVS_READONLY && strcmp(name,"hpanel_ui")==0)
        read_only_mode_count++;
    if(settings_result==ESP_OK) *handle=123;
    return settings_result;
}
void nvs_close(nvs_handle_t h){if(h==123)close_count++;}

static void run_cases(void) {
    reset_env(); CHECK("pending healthy",true,1,0);
    if(open_count!=1 || close_count!=1 || read_only_mode_count!=1 ||
       state_read_count!=1 || partition_read_count!=7 || health_alloc_count!=2) exit(2);
    reset_env(); running=&partitions[6];CHECK("OTA1 healthy",true,1,0);
    reset_env(); running=&partitions[2]; CHECK("factory untouched",true,0,0);
    if(partition_read_count!=0 || open_count!=0)exit(3);
    reset_env(); state=ESP_OTA_IMG_VALID;CHECK("valid OTA untouched",true,0,0);
    if(partition_read_count!=0 || open_count!=0)exit(4);
    reset_env(); state=ESP_OTA_IMG_ABORTED;CHECK("aborted denied",false,0,0);
    reset_env(); state=ESP_OTA_IMG_NEW;CHECK("new state denied",false,0,0);
    reset_env(); state=ESP_OTA_IMG_INVALID;CHECK("invalid denied",false,0,0);
    reset_env(); state=ESP_OTA_IMG_UNDEFINED;CHECK("undefined denied",false,0,0);
    reset_env(); state_result=ESP_FAIL;CHECK("missing OTA state denied",false,0,0);
    reset_env(); hide_running=true;CHECK("missing running partition denied",false,0,0);
    reset_env(); hide_partition=true;CHECK("missing layout denied and rolled back",false,0,1);
    reset_env(); partitions[0].address=0xA000;CHECK("NVS moved denied",false,0,1);
    reset_env(); partitions[2].address=0x30000;CHECK("factory moved denied",false,0,1);
    reset_env(); partitions[3].address=0x600000;CHECK("diaglog moved denied",false,0,1);
    reset_env(); partitions[4].address=0x720000;CHECK("otadata moved denied",false,0,1);
    reset_env(); partitions[5].address=0x800000;CHECK("OTA0 moved denied",false,0,1);
    reset_env(); partitions[6].address=0xC20000;CHECK("OTA1 moved denied",false,0,1);
    reset_env(); desc_result=ESP_FAIL;CHECK("unreadable app descriptor denied",false,0,1);
    reset_env(); desc_project="untrusted_project";CHECK("wrong project denied",false,0,1);
    reset_env(); desc_version="other_version";CHECK("wrong version denied",false,0,1);
    reset_env(); settings_result=ESP_FAIL;CHECK("NVS access error denied",false,0,1);
    reset_env(); settings_result=ESP_ERR_NVS_NOT_FOUND;CHECK("unconfigured settings allowed",true,1,0);
    if(close_count!=0)exit(5);
    reset_env(); psram_available=false;CHECK("missing PSRAM denied",false,0,1);
    reset_env(); internal_largest=4000;CHECK("internal heap below fixture limit denied",false,0,1);
    reset_env(); psram_largest=100;CHECK("PSRAM below fixture limit denied",false,0,1);
    reset_env(); fallback_possible=false;CHECK("no fallback denied without rollback",false,0,0);
    reset_env(); confirm_result=ESP_FAIL;CHECK("failed confirmation denied",false,1,0);
    reset_env(); running=&partitions[0];CHECK("data partition denied",false,0,0);
    reset_env(); running=&partitions[2];
    CHECK("factory skipped",true,0,0);
}
int main(void){run_cases();printf("PATCH116D_NATIVE_ADAPTER_HOST_CASES_PASS=%d\n",tests);return 0;}
