#include "panel_homey_favorites.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
static panel_homey_favorites_public_t s_public;
static bool tf(const cJSON *o,const char*k){const cJSON*v=cJSON_GetObjectItemCaseSensitive((cJSON*)o,k);return cJSON_IsTrue(v);}
static bool favorite(const cJSON*d){if(tf(d,"favorite")||tf(d,"favourite")||tf(d,"isFavorite")||tf(d,"isFavourite"))return true;const cJSON*s=cJSON_GetObjectItemCaseSensitive((cJSON*)d,"settings");return cJSON_IsObject(s)&&(tf(s,"favorite")||tf(s,"favourite"));}
static const cJSON *onoff(const cJSON*d){const cJSON*c=cJSON_GetObjectItemCaseSensitive((cJSON*)d,"capabilitiesObj");if(!cJSON_IsObject(c))return NULL;const cJSON*x=cJSON_GetObjectItemCaseSensitive((cJSON*)c,"onoff");if(cJSON_IsObject(x))return x;cJSON_ArrayForEach(x,c){if(!cJSON_IsObject(x))continue;const cJSON*i=cJSON_GetObjectItemCaseSensitive((cJSON*)x,"id");if(cJSON_IsString(i)&&i->valuestring&&strcmp(i->valuestring,"onoff")==0)return x;}return NULL;}
static void consider(const cJSON*d){if(s_public.count>=PANEL_HOMEY_FAVORITE_LIMIT||!cJSON_IsObject(d)||!favorite(d))return;const cJSON*c=onoff(d);if(!cJSON_IsObject(c))return;const cJSON*v=cJSON_GetObjectItemCaseSensitive((cJSON*)c,"value");if(!cJSON_IsBool(v))return;const cJSON*n=cJSON_GetObjectItemCaseSensitive((cJSON*)d,"name");if(!cJSON_IsString(n)||!n->valuestring||n->valuestring[0]=='\0')return;panel_homey_favorite_public_t*i=&s_public.items[s_public.count];memset(i,0,sizeof(*i));if(strlcpy(i->name,n->valuestring,sizeof(i->name))>=sizeof(i->name)){memset(i,0,sizeof(*i));return;}const cJSON*a=cJSON_GetObjectItemCaseSensitive((cJSON*)d,"available");i->available=!cJSON_IsFalse(a);i->onoff_known=true;i->onoff=cJSON_IsTrue(v);s_public.count++;}
panel_homey_favorites_result_t panel_homey_favorites_parse_and_publish(const char*j){if(!j||!j[0])return PANEL_HOMEY_FAVORITES_INVALID;cJSON*r=cJSON_Parse(j);if(!r)return PANEL_HOMEY_FAVORITES_PARSE_ERROR;panel_homey_favorites_public_t prev=s_public;memset(&s_public,0,sizeof(s_public));cJSON*d=cJSON_GetObjectItemCaseSensitive(r,"result");if(!d)d=r;if(!cJSON_IsArray(d)&&!cJSON_IsObject(d)){s_public=prev;cJSON_Delete(r);return PANEL_HOMEY_FAVORITES_PARSE_ERROR;}const cJSON*x=NULL;cJSON_ArrayForEach(x,d)consider(x);cJSON_Delete(r);return PANEL_HOMEY_FAVORITES_OK;}
bool panel_homey_favorites_copy_public(panel_homey_favorites_public_t*out){if(!out)return false;*out=s_public;return true;}


bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model)
{
    if (model == NULL) return false;
    for (size_t slot = 0U; slot < PANEL_HOMEY_FAVORITE_LIMIT; ++slot) {
        const size_t widget = 4U + slot;
        const panel_homey_favorite_public_t *item = slot < s_public.count ? &s_public.items[slot] : NULL;
        const char *fallback = slot == 0U ? "Belysning 1" : "Belysning 2";
        (void)snprintf(model->widget_title[widget], sizeof(model->widget_title[widget]), "%s", item != NULL ? item->name : fallback);
        model->widget_has_boolean[widget] = false;
        model->widget_boolean_value[widget] = false;
        if (item == NULL) model->widget_status[widget] = PANEL_WIDGET_UNCONFIGURED;
        else if (!item->available) model->widget_status[widget] = PANEL_WIDGET_UNAVAILABLE;
        else if (!item->onoff_known) model->widget_status[widget] = PANEL_WIDGET_UNKNOWN;
        else { model->widget_status[widget] = PANEL_WIDGET_AVAILABLE; model->widget_has_boolean[widget] = true; model->widget_boolean_value[widget] = item->onoff; }
    }
    return true;
}
