#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "panel_homey_favorites.h"
#include "lvgl.h"
bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *m){(void)m;return false;}
bool panel_homey_favorites_light_toggle_execution_ready(size_t i,bool r){(void)i;(void)r;return false;}
#include "../panel_ui.c"
static uint8_t pixels[480*40*2];
static uint8_t framebuffer[480*480*2];
static unsigned reads,mutations,frames;
static bool present=true;
static panel_information_t fixture;
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *p)
{
    unsigned w=(unsigned)(a->x2-a->x1+1);
    for(int y=a->y1;y<=a->y2;y++) memcpy(framebuffer+2*(y*480+a->x1),p+2*(y-a->y1)*w,2*w);
    frames++;lv_display_flush_ready(d);
}
static bool read_info(void *ctx,panel_information_kind_t k,panel_information_t *i)
{(void)ctx;(void)k;reads++;*i=fixture;return present;}
static void mutate(void *p){(void)p;mutations++;}
static void settings(void *p,const panel_ui_settings_t *s){(void)p;(void)s;mutations++;}
static lv_obj_t *button(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class) && strcmp(lv_label_get_text(root),text)==0)
        return lv_obj_get_parent(root);
    for(unsigned n=0;n<lv_obj_get_child_count(root);n++){
        lv_obj_t *b=button(lv_obj_get_child(root,n),text);if(b)return b;
    }
    return NULL;
}
static void draw(void){lv_tick_inc(20);lv_timer_handler();lv_refr_now(NULL);}
static void capture(unsigned index)
{
    char file[64];snprintf(file,sizeof(file),"information_%u.rgb565",index);
    FILE *f=fopen(file,"wb");assert(f);assert(fwrite(framebuffer,1,sizeof(framebuffer),f)==sizeof(framebuffer));fclose(f);
}
static void click(lv_obj_t *b){assert(b);lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);draw();}
int main(void)
{
    lv_init();lv_display_t *d=lv_display_create(480,480);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,flush);
    panel_ui_model_t m;panel_ui_model_init(&m,0);panel_ui_t *ui=NULL;
    panel_ui_config_t cfg={.model=&m,.callbacks={.read_information=read_info,
        .request_wifi_reconfigure=mutate,.request_wifi_cancel=mutate,
        .request_choose_homey=mutate,.request_homey_wipe=mutate,
        .request_change_athom_account=mutate,.settings_changed=settings}};
    assert(panel_ui_create(&ui,&cfg)&&panel_ui_activate(ui));draw();
    assert(panel_ui_open_settings(ui));click(button(ui->settings_main_body,"Avancerat"));
    const char *labels[]={"Wi-Fi-status","Homey-anslutning · Inte redo","Firmwareversion","Tidssynkronisering","Systemdiagnostik"};
    fixture.wifi_known=fixture.provisioning_known=fixture.homey_known=true;
    fixture.homey_state=2;fixture.wifi_online=true;fixture.now_ms=5000;
    fixture.firmware_known=true;strcpy(fixture.version,"fixture-Patch087");strcpy(fixture.idf_version,"v6.0.1");
    strcpy(fixture.build_date,"Oct 17 2026");strcpy(fixture.build_time,"12:00:00");
    fixture.time_known=fixture.time_synchronized=fixture.swedish_timezone=true;
    fixture.memory_known=true;fixture.internal_free=24000;fixture.internal_minimum=1500;fixture.psram_free=6000000;
    panel_information_observe_snapshot(&fixture.snapshot,PANEL_HOMEY_READ_OK,4000,2);
    for(unsigned k=0;k<5;k++){
        unsigned before=reads;click(button(ui->settings_advanced_body,labels[k]));
        assert(ui->modal_kind==MODAL_INFORMATION&&reads==before+1);
        assert(!lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
        assert(lv_obj_has_flag(ui->wifi_password,LV_OBJ_FLAG_HIDDEN));
        assert(lv_obj_has_flag(ui->wifi_cancel,LV_OBJ_FLAG_HIDDEN));
        assert(strcmp(lv_label_get_text(ui->status_title),panel_information_title(k))==0);
        assert(lv_obj_get_style_text_font(ui->status_text,0)==&homey_panel_font_22);
        assert(lv_obj_get_x(ui->status_dialog)>=0&&lv_obj_get_y(ui->status_dialog)>=0);
        assert(lv_obj_get_width(ui->status_dialog)<=480&&lv_obj_get_height(ui->status_dialog)<=480);
        click(button(ui->settings_advanced_body,"Ändra Wi-Fi"));assert(mutations==0);
        click(ui->information_refresh);assert(reads==before+2);
        assert(lv_obj_get_height(lv_obj_get_child(ui->information_refresh,0))<=30);
        capture(k);
        if(k==4){assert(lv_obj_get_scroll_bottom(ui->status_body)>0);lv_obj_scroll_to_y(ui->status_body,1000,LV_ANIM_OFF);draw();capture(5);}
        click(k%2?ui->status_close_footer:ui->status_close);assert(ui->modal_kind==MODAL_NONE);
    }
    click(button(ui->settings_advanced_body,labels[0]));fixture.wifi_online=false;
    click(ui->information_refresh);assert(strstr(lv_label_get_text(ui->status_text),"Frånkopplad"));
    fixture.wifi_online=true;click(ui->information_refresh);assert(strstr(lv_label_get_text(ui->status_text),"Ansluten"));
    present=false;click(ui->information_refresh);assert(strcmp(lv_label_get_text(ui->status_text),"Ej tillgänglig")==0);
    click(ui->status_close);present=true;
    /* Wi-Fi QR owns its existing object throughout info/error/open/close. */
    lv_obj_t *qr=lv_qrcode_create(ui->screen);lv_qrcode_set_size(qr,148);
    assert(panel_ui_update_wifi_provisioning(ui,qr,"fixture-setup","synthetic-code",true));
    assert(panel_ui_present_wifi_provisioning(ui,qr,false));draw();
    assert(ui->modal_kind==MODAL_WIFI&&lv_obj_has_flag(ui->information_refresh,LV_OBJ_FLAG_HIDDEN));
    click(ui->status_close);click(button(ui->settings_advanced_body,labels[4]));
    assert(ui->modal_kind==MODAL_INFORMATION&&lv_obj_has_flag(ui->wifi_qr,LV_OBJ_FLAG_HIDDEN));
    click(ui->status_close);assert(panel_ui_present_wifi_provisioning(ui,qr,false));click(ui->status_close);
    assert(panel_ui_clear_wifi_provisioning(ui,qr));
    click(button(ui->settings_advanced_body,labels[0]));click(ui->status_close);
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    for(unsigned n=0;n<100;n++){click(button(ui->settings_advanced_body,labels[0]));click(ui->information_refresh);click(ui->status_close);}
    lv_mem_monitor(&after);assert(after.free_size==before.free_size);
    assert(mutations==0);
    /* Existing settings feedback/error and non-information modal priority. */
    click(ui->settings_back_button);
    panel_ui_settings_t confirmed=m.settings;
    lv_obj_t *brightness=lv_obj_get_parent(ui->normal_brightness_label);
    lv_color_t normal=lv_obj_get_style_bg_color(brightness,LV_PART_MAIN);
    lv_obj_add_state(brightness,LV_STATE_PRESSED);draw();
    assert(!lv_color_eq(normal,lv_obj_get_style_bg_color(brightness,LV_PART_MAIN)));
    lv_obj_remove_state(brightness,LV_STATE_PRESSED);lv_tick_inc(300);draw();
    assert(lv_color_eq(normal,lv_obj_get_style_bg_color(brightness,LV_PART_MAIN)));
    click(brightness);assert(mutations==1&&m.settings.normal_brightness!=confirmed.normal_brightness);
    assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING)&&ui->modal_kind==MODAL_NONE);
    assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVED)&&ui->modal_kind==MODAL_NONE);
    click(button(ui->settings_main_body,"Avancerat"));click(button(ui->settings_advanced_body,labels[0]));
    assert(panel_ui_restore_settings(ui,&confirmed));
    assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_FAILED));
    assert(ui->modal_kind==MODAL_INFORMATION&&ui->deferred_settings_error);
    click(ui->status_close);assert(ui->modal_kind==MODAL_ERROR);
    assert(lv_obj_has_flag(ui->information_refresh,LV_OBJ_FLAG_HIDDEN));
    click(ui->status_close_footer);assert(ui->modal_kind==MODAL_NONE);
    assert(panel_ui_close_settings(ui));assert(panel_ui_select_page(ui,1,false));assert(panel_ui_select_page(ui,0,false));
    assert(mutations==1);
    printf("PATCH087_REAL_LVGL_FIVE_VIEWS_REFRESH_QR_LIFECYCLE PASS frames=%u cycles=100 pool_delta=%ld\n",frames,(long)after.free_size-(long)before.free_size);
    panel_ui_destroy(&ui);assert(!ui);lv_display_delete(d);lv_deinit();
}
