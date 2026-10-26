/* TEST-ONLY ABI declaration for distro libcjson.so.1 on Linux hosts.
 * Target ESP-IDF always uses its managed espressif__cjson component.
 * Prefer the actual pinned cJSON source/header in managed_components when available.
 * Based on MIT-licensed cJSON 1.7.19 public header (Dave Gamble et al.).
 */
#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
typedef int cJSON_bool;
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
void cJSON_Delete(cJSON *item);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
cJSON_bool cJSON_IsObject(const cJSON *item);
cJSON_bool cJSON_IsString(const cJSON *item);
cJSON_bool cJSON_IsNumber(const cJSON *item);
#ifdef __cplusplus
}
#endif
