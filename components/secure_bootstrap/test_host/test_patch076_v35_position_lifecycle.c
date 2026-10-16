/* Offline characterization of actual JSON -> alias -> snapshot -> model -> LVGL.
 * All Homey IDs are synthetic fixtures. No transport or NVS code is enabled. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "panel_homey_alias_store.h"
#include "panel_homey_read_snapshot.h"
#include "panel_homey_dashboard_binding.h"
#include "panel_homey_favorites.h"
#include "lvgl.h"

bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *m) {(void)m; return false;}
bool panel_homey_favorites_light_toggle_execution_ready(size_t i, bool r)
{(void)i; (void)r; return false;}
#define ESP_PLATFORM
#include "../panel_ui.c"

static panel_homey_alias_record_t record;
static panel_homey_alias_runtime_t aliases;
static panel_homey_snapshot_store_t store;
static panel_homey_dashboard_state_t dashboard;
static panel_ui_model_t model;
static panel_ui_t *ui;
static uint16_t pixels[480*160];
static unsigned checks;
static const panel_homey_alias_provider_t provider = {
    .context=&aliases, .capture=panel_homey_alias_runtime_capture,
    .resolve=panel_homey_alias_runtime_resolve,
};
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *p)
{(void)a; (void)p; lv_display_flush_ready(d);}
static void activate(void)
{
    assert(panel_homey_alias_runtime_activate(&aliases,&record,"fixture-homey") == PANEL_HOMEY_ALIAS_STORE_OK);
}
static void publish(uint64_t now, const char *a, const char *b, bool missing, bool available)
{
    char json[1000];
    snprintf(json,sizeof(json),
        "[{\"id\":\"fixture-cover-1\",\"available\":%s,\"capabilitiesObj\":{%s}},"
        "{\"id\":\"fixture-cover-2\",\"available\":%s,\"capabilitiesObj\":{%s}},"
        "{\"id\":\"fixture-light-1\",\"available\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":false}}}]",
        available?"true":"false",missing?"":a, available?"true":"false",missing?"":b);
    assert(panel_homey_snapshot_publish_json(&store,json,&provider,now)==PANEL_HOMEY_READ_OK);
}
static void value(uint64_t now, double v)
{
    char cap[150]; snprintf(cap,sizeof(cap),"\"windowcoverings_set\":{\"value\":%.2f}",v);
    publish(now,cap,cap,false,true);
}
static void apply(uint64_t now)
{
    panel_homey_read_snapshot_t snap;
    panel_homey_read_result_t result=panel_homey_snapshot_copy(&store,now,&snap);
    assert(panel_homey_dashboard_apply_snapshot(result,result==PANEL_HOMEY_READ_OK?&snap:NULL,now,&dashboard)
           !=PANEL_HOMEY_DASHBOARD_APPLY_INVALID);
    (void)panel_ui_apply_homey_dashboard_state(&model,&dashboard);
    assert(panel_ui_refresh(ui));
}
static void lights(void)
{
    for(unsigned i=4;i<6;i++) {
        assert(model.widget_status[i]==PANEL_WIDGET_AVAILABLE);
        assert(model.widget_has_boolean[i] && model.widget_boolean_value[i]);
        assert(strcmp(lv_label_get_text(ui->widget_status[i]),"Tänd")==0);
    }
}
static void visible(bool valid, unsigned step, const char *text)
{
    for(unsigned i=1;i<3;i++) {
        assert(model.roller_position_valid[i]==valid);
        assert(strcmp(lv_label_get_text(ui->widget_status[i]),text)==0);
        assert(lv_obj_has_flag(ui->widget_status[i],LV_OBJ_FLAG_HIDDEN)==valid);
        for(unsigned bar=0;bar<4;bar++) {
            assert(lv_obj_has_flag(ui->roller_lines[i][bar],LV_OBJ_FLAG_HIDDEN)==!valid);
            assert(lv_obj_has_flag(ui->roller_fills[i][bar],LV_OBJ_FLAG_HIDDEN)==!(valid && bar<step));
        }
        if(valid) assert(model.roller_position_step[i]==step && model.widget_status[i]==PANEL_WIDGET_AVAILABLE);
    }
    lights();checks++;
}
#include "poll_actual.inc"
int main(void)
{
    record.generation=7;record.entry_count=5;
    assert(panel_homey_alias_sha256("fixture-homey",record.homey_identity_digest));
    const unsigned indices[]={0,1,2,4,5};
    const char *ids[]={"fixture-state","fixture-cover-1","fixture-cover-2","fixture-light-1","fixture-light-2"};
    for(unsigned n=0;n<5;n++) {
        record.entries[n].dashboard_binding_index=indices[n];
        strcpy(record.entries[n].raw_device_id,ids[n]);
        strcpy(record.entries[n].raw_capability_id,n==0?"windowcoverings_state":n<3?"windowcoverings_set":"onoff");
    }
    activate();panel_homey_snapshot_store_init(&store,NULL,NULL,NULL);panel_homey_dashboard_state_init(&dashboard);
    lv_init();lv_display_t *display=lv_display_create(480,480);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    panel_ui_model_init(&model,0);const panel_ui_config_t config={.model=&model};
    assert(panel_ui_create(&ui,&config));assert(panel_ui_activate(ui));assert(panel_ui_set_homey_data_ready(ui,true));
    for(unsigned i=4;i<6;i++) {model.widget_status[i]=PANEL_WIDGET_AVAILABLE;model.widget_has_boolean[i]=true;model.widget_boolean_value[i]=true;}
    apply(0);visible(false,0,"Okänd"); /* Initial missing snapshot. */
    for(unsigned i=0;i<5;i++) {value(1000+i*1000,i*0.25);apply(1000+i*1000);visible(true,4-i,"");}
    uint32_t generation=store.buffers[store.active_index].generation;
    /* No publisher call on transport failure; existing successful data retained. */
    apply(35000);visible(true,0,"");assert(store.buffers[store.active_index].generation==generation);
    assert(panel_homey_snapshot_publish_json(&store,"{bad",&provider,45000)==PANEL_HOMEY_READ_INVALID);
    apply(45000);visible(true,0,"");assert(store.buffers[store.active_index].generation==generation);
    assert(store.last_publish.result==PANEL_HOMEY_READ_INVALID);
    apply(125000);visible(true,0,""); /* Exactly120000ms is still fresh. */
    apply(125001);visible(false,0,"Okänd");assert(model.homey_snapshot_stale);
    value(126000,0.25);apply(126000);visible(true,3,"");assert(!model.homey_snapshot_stale);
    /* A successful but missing-capability response replaces earlier data immediately. */
    publish(127000,"","",true,true);apply(127000);visible(false,0,"Okänd");assert(!model.homey_snapshot_stale);
    assert(model.widget_status[1]==PANEL_WIDGET_UNKNOWN && model.widget_status[2]==PANEL_WIDGET_UNKNOWN);
    value(128000,0.5);apply(128000);visible(true,2,"");
    publish(129000,"\"windowcoverings_set\":{\"value\":null}","\"windowcoverings_set\":{\"value\":null}",false,true);
    apply(129000);visible(false,0,"Okänd");assert(model.widget_status[1]==PANEL_WIDGET_AVAILABLE);
    publish(129500,"\"windowcoverings_set\":{\"value\":\"bad\"}","\"windowcoverings_set\":{\"value\":0.5}",false,true);
    apply(129500);
    assert(!model.roller_position_valid[1] && model.roller_position_valid[2]);
    assert(strcmp(lv_label_get_text(ui->widget_status[1]),"Okänd")==0);
    assert(lv_obj_has_flag(ui->widget_status[2],LV_OBJ_FLAG_HIDDEN));
    assert(model.roller_position_step[2]==2);lights();checks++;
    value(130000,1.1);apply(130000);visible(false,0,"Okänd");
    publish(131000,"\"windowcoverings_set\":{\"value\":0.5}","\"windowcoverings_set\":{\"value\":0.5}",false,false);
    apply(131000);visible(false,0,"Otillgänglig");assert(model.widget_status[1]==PANEL_WIDGET_UNAVAILABLE);
    value(132000,0.75);apply(132000);visible(true,1,"");
    /* Actual digest mismatch rejection; no real NVS operation. */
    assert(panel_homey_alias_runtime_activate(&aliases,&record,"fixture-wrong-homey")!=PANEL_HOMEY_ALIAS_STORE_OK);
    assert(panel_homey_snapshot_publish_json(&store,"[{\"id\":\"fixture-cover-1\",\"available\":true,\"capabilitiesObj\":{\"windowcoverings_set\":{\"value\":0.5}}}]",&provider,133000)==PANEL_HOMEY_READ_NOT_CONFIGURED);
    apply(133000);visible(true,1,"");apply(252001);visible(false,0,"Okänd");
    activate();value(253000,0);apply(253000);visible(true,4,"");
    strcpy(record.entries[1].raw_device_id,"fixture-missing-1");strcpy(record.entries[2].raw_device_id,"fixture-missing-2");
    activate();value(254000,0.5);apply(254000);visible(false,0,"Okänd");
    strcpy(record.entries[1].raw_device_id,"fixture-cover-1");strcpy(record.entries[2].raw_device_id,"fixture-cover-2");
    activate();value(255000,0.5);apply(255000);visible(true,2,"");
    /* Readiness is a distinct render gate, even while model data remains valid. */
    assert(panel_ui_set_homey_data_ready(ui,false));assert(model.roller_position_valid[1]);
    assert(strcmp(lv_label_get_text(ui->widget_status[1]),"Okänd")==0);
    assert(strcmp(lv_label_get_text(ui->widget_status[4]),"Okänd")==0);
    assert(panel_ui_set_homey_data_ready(ui,true));visible(true,2,"");
    /* Stale -> fresh handoff must survive a single display-lock timeout. */
    test_lock_allowed=false;
    poll_homey_dashboard_if_due(376000);
    visible(true,2,""); /* No UI operation was possible while lock failed. */
    test_lock_allowed=true;
    poll_homey_dashboard_if_due(376100);
    visible(false,0,"Okänd"); /* Next successful poll must enforce stale. */
    value(377000,0.25);
    test_lock_allowed=false;
    poll_homey_dashboard_if_due(377200);
    visible(false,0,"Okänd");
    test_lock_allowed=true;
    poll_homey_dashboard_if_due(377300);
    visible(true,3,"");
    printf("V35_REAL_PARSER_ALIAS_SNAPSHOT_MODEL_LVGL PASS checkpoints=%u\n",checks);
    return 0;
}
