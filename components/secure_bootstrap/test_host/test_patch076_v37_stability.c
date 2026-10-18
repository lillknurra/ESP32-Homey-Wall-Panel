/* Synthetic IDs; real cJSON/Favorites/model/LVGL, no transport or device access. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "cJSON.h"
#include "panel_homey_favorites.h"
static unsigned text_writes, style_writes, scroll_writes, full_refreshes, invalidations;
static void counted_text(lv_obj_t *o,const char *s){text_writes++;lv_label_set_text(o,s);}
static void counted_color(lv_obj_t *o,lv_color_t c,lv_style_selector_t s){style_writes++;lv_obj_set_style_bg_color(o,c,s);}
static void counted_opa(lv_obj_t *o,lv_opa_t v,lv_style_selector_t s){style_writes++;lv_obj_set_style_bg_opa(o,v,s);}
static void counted_font(lv_obj_t *o,const lv_font_t *v,lv_style_selector_t s){style_writes++;lv_obj_set_style_text_font(o,v,s);}
static void counted_align(lv_obj_t *o,lv_text_align_t v,lv_style_selector_t s){style_writes++;lv_obj_set_style_text_align(o,v,s);}
static void counted_scroll(lv_obj_t *o,int32_t x,lv_anim_enable_t a){scroll_writes++;lv_obj_scroll_to_x(o,x,a);}
static void counted_refr(lv_display_t *d){full_refreshes++;lv_refr_now(d);}
static lv_result_t test_lv_qrcode_update(lv_obj_t *obj, const void *data, uint32_t data_len);
#define lv_label_set_text counted_text
#define lv_obj_set_style_bg_color counted_color
#define lv_obj_set_style_bg_opa counted_opa
#define lv_obj_set_style_text_font counted_font
#define lv_obj_set_style_text_align counted_align
#define lv_obj_scroll_to_x counted_scroll
#define lv_refr_now counted_refr
#define PANEL_UI_QR_UPDATE(qr,payload,length) test_lv_qrcode_update((qr),(payload),(length))
#define ESP_PLATFORM
#include "ui_actual.inc"
#undef lv_label_set_text
#undef lv_obj_set_style_bg_color
#undef lv_obj_set_style_bg_opa
#undef lv_obj_set_style_text_font
#undef lv_obj_set_style_text_align
#undef lv_obj_scroll_to_x
#undef lv_refr_now
#undef PANEL_UI_QR_UPDATE
static panel_ui_model_t model;
static panel_ui_t *ui;
static uint16_t pixels[480*160];
static bool measure_only;
static uint8_t last_brightness;
static bool forbidden_light_command(void *context,size_t widget,bool value){(void)context;(void)widget;(void)value;assert(!"Host test must never dispatch a device command");return false;}
static void brightness(void *context,uint8_t value){(void)context;last_brightness=value;}
static unsigned allocation_observed, action;
static unsigned wifi_cancel_requests;
static unsigned settings_change_requests;
static panel_ui_settings_t last_settings_request;
static bool fail_next_qr_update;
static lv_result_t test_lv_qrcode_update(lv_obj_t *obj, const void *data, uint32_t data_len)
{
 if(fail_next_qr_update){fail_next_qr_update=false;return LV_RESULT_INVALID;}
 return lv_qrcode_update(obj,data,data_len);
}
static void request_wifi_cancel(void *context){(void)context;wifi_cancel_requests++;}
static void settings_changed(void *context,const panel_ui_settings_t *settings){(void)context;assert(settings);last_settings_request=*settings;settings_change_requests++;}
static const char *user="{\"properties\":{\"favoriteDevices\":[\"one\",\"two\"]}}";
static const char *devices="[{\"id\":\"one\",\"name\":\"Ytterbelysning\",\"available\":true,\"capabilities\":[\"onoff\"],\"capabilitiesObj\":{\"onoff\":{\"value\":false,\"type\":\"boolean\",\"getable\":true,\"setable\":true}}},{\"id\":\"two\",\"name\":\"Dekorbelysning\",\"available\":true,\"capabilities\":[\"onoff\"],\"capabilitiesObj\":{\"onoff\":{\"value\":false,\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}]";
static panel_homey_read_result_t resolve(void *c,const char *id,const char *cap,char *da,size_t ds,char *ca,size_t cs){(void)c;assert(!strcmp(cap,"onoff"));snprintf(da,ds,"light_%s",!strcmp(id,"one")?"1":"2");snprintf(ca,cs,"on");return PANEL_HOMEY_READ_OK;}
static const panel_homey_alias_provider_t provider={.resolve=resolve};
/* Only the pre-transport production identity boundary is executed. */
typedef struct {struct {char id[128];} selected_homey;} athom_cloud_state_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
static void diagnostic_set(const char *stage,int error){(void)stage;(void)error;}
static void athom_cloud_alias_invalidate(void){panel_homey_favorites_revoke_light_toggle_authorization();}
#include "identity_actual.inc"

static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *p){(void)a;(void)p;lv_display_flush_ready(d);}
static void invalid_event(lv_event_t *e){(void)e;invalidations++;}
static void names(void){assert(!strcmp(model.widget_title[4],"Ytterbelysning"));assert(!strcmp(model.widget_title[5],"Dekorbelysning"));for(unsigned i=4;i<6;i++)assert(!strcmp(lv_label_get_text(ui->widget_status[i]),"Släckt"));}
static void *observing_malloc(size_t n){
 if(!allocation_observed++){
  panel_homey_favorites_public_t s;assert(panel_homey_favorites_copy_public(&s));
  if(!measure_only){assert(s.state==PANEL_HOMEY_FAVORITES_VALID_CONFIGURED && s.count==2);assert(!s.items[0].light_toggle_authorized);assert(!panel_homey_favorites_light_toggle_execution_ready(4,true));}
  (void)panel_homey_favorites_apply_ui_model(&model);assert(panel_ui_refresh(ui));
  if(!measure_only)names();
  if(action==1)panel_homey_favorites_clear();
  if(action==2)panel_homey_favorites_revoke_light_toggle_authorization();
 }
 return malloc(n);
}
static void reset_counts(void){text_writes=style_writes=scroll_writes=full_refreshes=invalidations=0;}
static void test_wifi_provisioning_modal(void)
{
 panel_ui_callbacks_t callbacks={.request_brightness=brightness,.request_light_toggle=forbidden_light_command,.request_wifi_cancel=request_wifi_cancel,.settings_changed=settings_changed};
 panel_ui_destroy(&ui);panel_ui_config_t config={.model=&model,.callbacks=callbacks};assert(panel_ui_create(&ui,&config));assert(panel_ui_activate(ui));
 /* Settings save first, then Wi-Fi provisioning on the same production UI. */
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
 assert(ui->modal_kind==MODAL_NONE && ui->save_pending);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVED));
 assert(ui->modal_kind==MODAL_NONE && !ui->save_pending);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
 lv_obj_t *qr=lv_qrcode_create(ui->screen);assert(qr);lv_qrcode_set_size(qr,96);
 assert(panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","SYNTHETIC8",true));
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));assert(ui->modal_kind==MODAL_WIFI);
 /* Wi-Fi first, settings save second, then return to the same session. */
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
 assert(ui->modal_kind==MODAL_WIFI && ui->save_pending);
 assert(!lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);
 assert(ui->modal_kind==MODAL_NONE && ui->save_pending);
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVED));
 assert(ui->modal_kind==MODAL_NONE && !ui->save_pending);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));
 assert(ui->modal_kind==MODAL_WIFI && !lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 assert(!strcmp(lv_label_get_text(ui->wifi_ssid),"Wi-Fi: HomeyPanel-Setup"));
 assert(!strcmp(lv_label_get_text(ui->wifi_password),"Lösenord: SYNTHETIC8"));
 assert(lv_obj_get_parent(qr)==ui->status_body);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_CLICKABLE));
 assert(!lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 /* Closing with X hides the modal only; the provisioner callback is untouched. */
 lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);assert(ui->modal_kind==MODAL_NONE);assert(wifi_cancel_requests==0U);
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));
 lv_obj_send_event(ui->wifi_cancel,LV_EVENT_CLICKED,NULL);assert(wifi_cancel_requests==1U);assert(!ui->wifi_cancel_allowed);
 /* Commit reservation hides secret presentation; a fresh session republishes
  * the matching QR and text together. */
 assert(panel_ui_set_wifi_cancel_pending(ui,true,true));assert(lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 assert(panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","ROTATED88",true));
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));
 assert(!strcmp(lv_label_get_text(ui->wifi_password),"Lösenord: ROTATED88"));
 assert(panel_ui_clear_wifi_provisioning(ui,qr));assert(lv_obj_get_parent(qr)==ui->screen);
 assert(!strcmp(lv_label_get_text(ui->wifi_password),""));assert(ui->modal_kind==MODAL_NONE);
 assert(panel_ui_set_wifi_reconfigure_result(ui,PANEL_UI_WIFI_CANCEL_FAILED));
 assert(ui->modal_kind==MODAL_ERROR);lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);
 assert(ui->modal_kind==MODAL_NONE);
 uint32_t child_count=lv_obj_get_child_count(ui->screen);
 for(unsigned cycle=0;cycle<20;cycle++){
  assert(panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","ROTATED88",true));
  assert(panel_ui_present_wifi_provisioning(ui,qr,false));
  lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);
  assert(ui->modal_kind==MODAL_NONE);
  assert(panel_ui_clear_wifi_provisioning(ui,qr));
  assert(lv_obj_get_child_count(ui->screen)==child_count);
 }
 /* A successful old publication followed by an injected QR failure during
  * rotation must hide and clear the prior session presentation. */
 assert(panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","ROTATED88",true));
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));
 fail_next_qr_update=true;
 assert(!panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","NEWSESSION9",true));
 assert(ui->modal_kind==MODAL_ERROR && ui->wifi_qr==qr);
 assert(lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 assert(!strcmp(lv_label_get_text(ui->wifi_ssid),""));
 assert(!strcmp(lv_label_get_text(ui->wifi_password),""));
 lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);
 assert(ui->modal_kind==MODAL_NONE);
 assert(panel_ui_update_wifi_provisioning(ui,qr,"HomeyPanel-Setup","RECOVER88",true));
 assert(panel_ui_present_wifi_provisioning(ui,qr,false));
 assert(!lv_obj_has_flag(qr,LV_OBJ_FLAG_HIDDEN));
 assert(!strcmp(lv_label_get_text(ui->wifi_password),"Lösenord: RECOVER88"));
 assert(panel_ui_clear_wifi_provisioning(ui,qr));
 lv_obj_delete(qr);
}
static void test_settings_feedback_and_save_flow(void)
{
 assert(panel_ui_open_settings(ui));
 lv_obj_t *button=lv_obj_get_parent(ui->normal_brightness_label);assert(button);
 lv_color_t normal=lv_obj_get_style_bg_color(button,LV_PART_MAIN);
 lv_obj_add_state(button,LV_STATE_PRESSED);
 lv_color_t pressed=lv_obj_get_style_bg_color(button,LV_PART_MAIN);
 assert(!lv_color_eq(normal,pressed));
 assert(lv_color_eq(pressed,lv_color_darken(normal,36)));
 lv_obj_remove_state(button,LV_STATE_PRESSED);
 assert(lv_color_eq(lv_obj_get_style_bg_color(button,LV_PART_MAIN),normal));

 panel_ui_settings_t confirmed=model.settings;
 const unsigned before=settings_change_requests;
 lv_obj_send_event(button,LV_EVENT_CLICKED,NULL);
 assert(settings_change_requests==before+1U);
 assert(model.settings.normal_brightness==last_settings_request.normal_brightness);
 assert(!strcmp(lv_label_get_text(ui->normal_brightness_label),"Normal ljusstyrka: 100 %"));
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
 assert(ui->save_pending && ui->modal_kind==MODAL_NONE);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVED));
 assert(!ui->save_pending && ui->modal_kind==MODAL_NONE);
 assert(lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));

 confirmed=model.settings;
 lv_obj_send_event(button,LV_EVENT_CLICKED,NULL);
 assert(model.settings.normal_brightness!=confirmed.normal_brightness);
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
 assert(panel_ui_restore_settings(ui,&confirmed));
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_FAILED));
 assert(model.settings.normal_brightness==confirmed.normal_brightness);
 assert(strstr(lv_label_get_text(ui->normal_brightness_label),"100 %"));
 assert(ui->modal_kind==MODAL_ERROR);
 assert(!strcmp(lv_label_get_text(ui->status_text),"Kunde inte spara"));
 lv_obj_send_event(ui->status_close,LV_EVENT_CLICKED,NULL);
 assert(ui->modal_kind==MODAL_NONE && lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));

 const unsigned burst_before=settings_change_requests;
 for(unsigned i=0;i<5U;i++) lv_obj_send_event(button,LV_EVENT_CLICKED,NULL);
 assert(settings_change_requests==burst_before+5U);
 assert(model.settings.normal_brightness==last_settings_request.normal_brightness);
 assert(!strcmp(lv_label_get_text(ui->normal_brightness_label),"Normal ljusstyrka: 100 %"));
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
 assert(ui->modal_kind==MODAL_NONE);
 assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVED));
 assert(panel_ui_close_settings(ui));
}

static lv_color_t roller_color(size_t widget,size_t line,bool fill)
{
 lv_obj_t *obj=fill?ui->roller_fills[widget][line]:ui->roller_lines[widget][line];
 assert(obj);return lv_obj_get_style_bg_color(obj,LV_PART_MAIN);
}
static void blink_tick(void)
{
 lv_tick_inc(100U);(void)lv_timer_handler();
}
static void assert_arrow_feedback(size_t widget,bool lower)
{
 assert(ui->roller_feedback_active && ui->roller_feedback_timer);
 const size_t selected=lower?1U:0U,other=lower?0U:1U;
 assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][selected],0),lv_color_hex(0x071018U)));
 assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][other],0),lv_color_hex(0x80919EU)));
 assert(lv_color_eq(lv_obj_get_style_bg_color(ui->roller_arrow_highlights[widget][selected],0),lv_color_hex(0x8BD3F7U)));
 assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][selected],0)==LV_OPA_COVER);
 assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][other],0)==LV_OPA_TRANSP);
 for(size_t line=0;line<4U;line++)
  assert(lv_color_eq(roller_color(widget,line,true),lv_color_hex(0x8BD3F7U)));
 lv_tick_inc(549U);(void)lv_timer_handler();
 assert(ui->roller_feedback_active && ui->roller_feedback_timer);
 lv_tick_inc(1U);(void)lv_timer_handler();
 assert(!ui->roller_feedback_active && ui->roller_feedback_timer==NULL);
 assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][selected],0),lv_color_hex(0x80919EU)));
 assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][selected],0)==LV_OPA_TRANSP);
 for(size_t line=0;line<4U;line++)
  assert(lv_color_eq(roller_color(widget,line,true),lv_color_hex(0x8BD3F7U)));
}
static void test_roller_single_arrow_feedback(void)
{
 assert(panel_ui_select_page(ui,0U,false));
 for(size_t widget=1U;widget<=2U;widget++) {
  model.widget_status[widget]=PANEL_WIDGET_AVAILABLE;
  model.roller_position_valid[widget]=true;
  model.roller_position_step[widget]=4U;
  model.homey_snapshot_stale=false;
  render_roller_widget(ui,widget);
 }
 const unsigned saves_before=settings_change_requests;
 const uint8_t step_before=model.roller_position_step[1];
 lv_obj_t *upper=lv_obj_get_child(ui->widget_card[1],1);assert(upper);
 lv_obj_t *lower=lv_obj_get_child(ui->widget_card[1],2);assert(lower);
 lv_obj_send_event(upper,LV_EVENT_CLICKED,NULL);
 assert_arrow_feedback(1U,false);
 lv_obj_send_event(lower,LV_EVENT_CLICKED,NULL);
 assert_arrow_feedback(1U,true);
 lv_obj_t *upper_b=lv_obj_get_child(ui->widget_card[2],1);assert(upper_b);
 lv_obj_t *lower_b=lv_obj_get_child(ui->widget_card[2],2);assert(lower_b);
 lv_obj_send_event(upper_b,LV_EVENT_CLICKED,NULL);
 assert_arrow_feedback(2U,false);
 lv_obj_send_event(lower_b,LV_EVENT_CLICKED,NULL);
 assert_arrow_feedback(2U,true);

 /* Repeated touch restarts the one bounded timer and restores the old half. */
 lv_obj_send_event(upper,LV_EVENT_CLICKED,NULL);lv_tick_inc(100U);(void)lv_timer_handler();
 lv_obj_send_event(lower,LV_EVENT_CLICKED,NULL);
 assert(!ui->roller_feedback_timer || ui->roller_feedback_lower_half);
 assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1U][1U],0),lv_color_hex(0x071018U)));
 assert(lv_color_eq(lv_obj_get_style_bg_color(ui->roller_arrow_highlights[1U][1U],0),lv_color_hex(0x8BD3F7U)));
 lv_tick_inc(549U);(void)lv_timer_handler();assert(ui->roller_feedback_timer!=NULL);
 lv_tick_inc(1U);(void)lv_timer_handler();
 assert(ui->roller_feedback_timer==NULL);
 /* A canceled press has no CLICKED callback; scrolling/page changes cancel a live pulse. */
 lv_obj_send_event(upper,LV_EVENT_PRESS_LOST,NULL);
 assert(ui->roller_feedback_timer==NULL);
 lv_obj_send_event(lower,LV_EVENT_CLICKED,NULL);
 assert(ui->roller_feedback_timer!=NULL);
 lv_obj_send_event(ui->pager,LV_EVENT_SCROLL_BEGIN,NULL);
 assert(ui->roller_feedback_timer==NULL);
 lv_obj_send_event(upper_b,LV_EVENT_CLICKED,NULL);
 assert(ui->roller_feedback_timer!=NULL);
 assert(panel_ui_select_page(ui,1U,false));
 assert(ui->roller_feedback_timer==NULL);
 assert(lv_color_eq(roller_color(2U,0U,true),lv_color_hex(0x8BD3F7U)));
 assert(model.roller_position_step[1]==step_before);
 assert(settings_change_requests==saves_before);
 assert(!lv_obj_has_flag(ui->widget_card[0],LV_OBJ_FLAG_CLICKABLE));
 assert(panel_ui_select_page(ui,0U,false));
}

int main(int argc,char **argv){
 measure_only=argc>1 && !strcmp(argv[1],"measure");bool favorites_only=argc>1 && !strcmp(argv[1],"favorites");lv_init();lv_display_t *d=lv_display_create(480,480);assert(d);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(d,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
 panel_ui_model_init(&model,0);assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);(void)panel_homey_favorites_apply_ui_model(&model);
 panel_ui_config_t conf={.model=&model,.callbacks={.request_brightness=brightness,.request_light_toggle=forbidden_light_command}};assert(panel_ui_create(&ui,&conf));assert(panel_ui_activate(ui));assert(panel_ui_set_homey_data_ready(ui,true));assert(panel_ui_refresh(ui));lv_refr_now(d);names();
 test_wifi_provisioning_modal();
 test_settings_feedback_and_save_flow();
 test_roller_single_arrow_feedback();
 assert(panel_ui_set_homey_data_ready(ui,true));assert(panel_ui_refresh(ui));names();
 lv_display_add_event_cb(d,invalid_event,LV_EVENT_INVALIDATE_AREA,NULL);reset_counts();
 for(unsigned i=0;i<5;i++){assert(panel_ui_refresh(ui));assert(panel_ui_set_homey_data_ready(ui,true));assert(panel_ui_set_connection(ui,&ui->connection));assert(panel_ui_set_time(ui,NULL,false));}
 printf("IDENTICAL_SEQUENCE text=%u styles=%u scroll=%u full=%u invalidation_events=%u\n",text_writes,style_writes,scroll_writes,full_refreshes,invalidations);
 if(!measure_only && !favorites_only)assert(text_writes==0 && style_writes==0 && scroll_writes==0 && full_refreshes==0 && invalidations==0);
 cJSON_Hooks hooks={observing_malloc,free};cJSON_InitHooks(&hooks);allocation_observed=0;assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);cJSON_InitHooks(NULL);assert(allocation_observed);assert(panel_ui_refresh(ui));names();
 if(measure_only){puts("BASELINE_MEASUREMENT_COMPLETE");return 0;}
 /* Production rotation reconciles authority even when no model value changed. */
 assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);
 allocation_observed=0;action=0;cJSON_InitHooks(&hooks);
 assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);cJSON_InitHooks(NULL);
 assert(!panel_homey_favorites_apply_ui_model(&model));assert(!lv_obj_has_flag(ui->widget_card[4],LV_OBJ_FLAG_CLICKABLE));
 reset_counts();assert(panel_ui_set_light_toggle_pending(ui,4,false));
 assert(lv_obj_has_flag(ui->widget_card[4],LV_OBJ_FLAG_CLICKABLE));assert(lv_obj_has_flag(ui->widget_card[5],LV_OBJ_FLAG_CLICKABLE));assert(text_writes==0 && style_writes==0 && invalidations==0);
 reset_counts();
 for(unsigned poll=0;poll<20;poll++){
  allocation_observed=0;action=0;cJSON_InitHooks(&hooks);
  assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);
  cJSON_InitHooks(NULL);assert(!panel_homey_favorites_apply_ui_model(&model));assert(panel_ui_refresh(ui));names();
 }
 printf("IDENTICAL_PUBLICATIONS polls=20 text=%u styles=%u scroll=%u invalidation_events=%u\n",text_writes,style_writes,scroll_writes,invalidations);
 assert(text_writes==0 && style_writes==0 && scroll_writes==0 && invalidations==0);
 assert(panel_homey_favorites_light_toggle_execution_ready(4,true));
 /* Explicit clear/identity epoch or authority revocation during parse cannot be undone. */
 for(action=1;action<=2;action++){
  allocation_observed=0;cJSON_InitHooks(&hooks);assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_INVALID);cJSON_InitHooks(NULL);assert(!panel_homey_favorites_light_toggle_execution_ready(4,true));
  if(action==1){assert(panel_homey_favorites_get_state()==PANEL_HOMEY_FAVORITES_UNVERIFIED);assert(panel_ui_refresh(ui));assert(!strcmp(lv_label_get_text(ui->widget_title[4]),"Belysning 1"));assert(!strcmp(lv_label_get_text(ui->widget_status[4]),"Okänd"));}
  assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);assert(panel_ui_refresh(ui));names();
 }
 assert(panel_homey_favorites_parse_and_publish("bad",devices)==PANEL_HOMEY_FAVORITES_PARSE_ERROR);assert(!panel_homey_favorites_light_toggle_execution_ready(4,true));assert(panel_ui_refresh(ui));assert(!strcmp(lv_label_get_text(ui->widget_status[4]),"Okänd"));
 assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);assert(panel_ui_refresh(ui));names();
 char changed[1200];snprintf(changed,sizeof(changed),"%s",devices);char *v=strstr(changed,"\"value\":false");assert(v);memcpy(v+8,"true ",5);assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,changed,&provider)==PANEL_HOMEY_FAVORITES_OK);reset_counts();assert(panel_ui_refresh(ui));assert(!strcmp(lv_label_get_text(ui->widget_status[4]),"Tänd"));assert(!strcmp(lv_label_get_text(ui->widget_status[5]),"Släckt"));printf("ONE_LIGHT_CHANGE text=%u styles=%u\n",text_writes,style_writes);assert(text_writes==1 && style_writes==2);
 model.widget_status[1]=PANEL_WIDGET_AVAILABLE;model.roller_position_valid[1]=true;model.roller_position_step[1]=2;model.homey_snapshot_stale=false;assert(panel_ui_refresh(ui));assert(lv_obj_has_flag(ui->widget_status[1],LV_OBJ_FLAG_HIDDEN));model.homey_snapshot_stale=true;assert(panel_ui_refresh(ui));assert(!strcmp(lv_label_get_text(ui->widget_status[1]),"Okänd"));
 struct tm t={.tm_hour=12,.tm_min=34};assert(panel_ui_set_time(ui,&t,true));assert(strstr(lv_label_get_text(ui->clock_label),"12:34"));reset_counts();assert(panel_ui_set_time(ui,&t,true));assert(text_writes==0);t.tm_min=35;assert(panel_ui_set_time(ui,&t,true));assert(text_writes==1);
 assert(panel_ui_select_page(ui,2,true));lv_tick_inc(30);lv_timer_handler();int32_t x=lv_obj_get_scroll_x(ui->pager);reset_counts();assert(panel_ui_refresh(ui));assert(lv_obj_get_scroll_x(ui->pager)==x && scroll_writes==0);for(unsigned i=0;i<30;i++){lv_tick_inc(20);lv_timer_handler();}assert(model.active_page==2);
 assert(panel_ui_open_settings(ui));assert(!lv_obj_has_flag(ui->settings_layer,LV_OBJ_FLAG_HIDDEN));
 lv_display_add_event_cb(d,invalid_event,LV_EVENT_INVALIDATE_AREA,NULL);reset_counts();
 for(unsigned refresh=0U;refresh<5U;refresh++)assert(panel_ui_refresh(ui));
 assert(text_writes==0&&style_writes==0&&scroll_writes==0&&full_refreshes==0&&invalidations==0);
 assert(panel_ui_close_settings(ui));
 model.power_state=PANEL_POWER_DIMMED;assert(panel_ui_refresh(ui));assert(last_brightness==model.settings.dimmed_brightness);model.power_state=PANEL_POWER_OFF;assert(panel_ui_refresh(ui));assert(last_brightness==0);model.power_state=PANEL_POWER_ACTIVE;assert(panel_ui_refresh(ui));assert(last_brightness==model.settings.normal_brightness);assert(ui->rendered_power==PANEL_POWER_ACTIVE);assert(full_refreshes>0);
 athom_cloud_state_t cloud={0};snprintf(cloud.selected_homey.id,sizeof(cloud.selected_homey.id),"fixture-homey-A");
 assert(select_identity_prefix(&cloud,"fixture-homey-A")==ESP_OK);assert(panel_homey_favorites_get_state()==PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);assert(!panel_homey_favorites_light_toggle_execution_ready(4,true));
 assert(panel_homey_favorites_parse_and_publish_with_alias_provider(user,devices,&provider)==PANEL_HOMEY_FAVORITES_OK);
 assert(select_identity_prefix(&cloud,"fixture-homey-B")==ESP_OK);assert(panel_homey_favorites_get_state()==PANEL_HOMEY_FAVORITES_UNVERIFIED);assert(panel_ui_refresh(ui));assert(!strcmp(lv_label_get_text(ui->widget_title[4]),"Belysning 1"));assert(!strcmp(lv_label_get_text(ui->widget_status[4]),"Okänd"));
 assert(panel_ui_set_homey_data_ready(ui,false));assert(!panel_light_card_actionable(ui,4));assert(panel_ui_set_homey_data_ready(ui,true));
 puts("V37_ATOMIC_PUBLICATION_AND_CHANGE_DRIVEN_REAL_LVGL PASS");return 0;
}
