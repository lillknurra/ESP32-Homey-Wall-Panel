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
static unsigned settings_calls;
static panel_ui_settings_t last_saved_settings;
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
static void settings(void *p,const panel_ui_settings_t *s)
{(void)p;assert(s);last_saved_settings=*s;settings_calls++;}
static lv_obj_t *button(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class) && strcmp(lv_label_get_text(root),text)==0)
        return lv_obj_get_parent(root);
    for(unsigned n=0;n<lv_obj_get_child_count(root);n++){
        lv_obj_t *b=button(lv_obj_get_child(root,n),text);if(b)return b;
    }
    return NULL;
}
static lv_obj_t *find_label(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class)&&strcmp(lv_label_get_text(root),text)==0)return root;
    for(unsigned n=0;n<lv_obj_get_child_count(root);n++){
        lv_obj_t *found=find_label(lv_obj_get_child(root,n),text);if(found)return found;
    }
    return NULL;
}
static lv_obj_t *find_line(lv_obj_t *root)
{
    if(lv_obj_check_type(root,&lv_line_class))return root;
    for(unsigned n=0;n<lv_obj_get_child_count(root);n++){
        lv_obj_t *found=find_line(lv_obj_get_child(root,n));if(found)return found;
    }
    return NULL;
}
static void assert_color(lv_obj_t *obj,uint32_t rgb)
{assert(obj&&lv_color_eq(lv_obj_get_style_bg_color(obj,0),lv_color_hex(rgb)));}
static void assert_theme_labels_valid(panel_ui_t *ui)
{
    assert(ui&&ui->theme_label_count>0U&&ui->theme_label_count<THEME_LABEL_LIMIT);
    for(size_t i=0;i<ui->theme_label_count;i++){
        assert(ui->theme_labels[i].label);
        assert(lv_obj_is_in_widget_tree(ui->theme_labels[i].label));
        assert(lv_obj_check_type(ui->theme_labels[i].label,&lv_label_class));
        for(size_t j=i+1U;j<ui->theme_label_count;j++)
            assert(ui->theme_labels[i].label!=ui->theme_labels[j].label);
    }
}
static void draw(void){lv_tick_inc(20);lv_timer_handler();lv_refr_now(NULL);}
static void capture(unsigned index)
{
    char file[64];snprintf(file,sizeof(file),"information_%u.rgb565",index);
    FILE *f=fopen(file,"wb");assert(f);assert(fwrite(framebuffer,1,sizeof(framebuffer),f)==sizeof(framebuffer));fclose(f);
}
static void click(lv_obj_t *b){assert(b);lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);draw();}
static lv_color_t roller_fill_color(panel_ui_t *ui,uint8_t widget,size_t line)
{ return lv_obj_get_style_bg_color(ui->roller_fills[widget][line],0); }
static void assert_single_arrow_feedback(panel_ui_t *ui,uint8_t widget,bool lower,
                                         panel_background_mode_t theme)
{
    assert(ROLLER_TOUCH_FEEDBACK_MS==550U);
    ui->model->settings.background_mode=theme;render_settings(ui);
    const uint32_t highlight=theme==PANEL_THEME_LIGHT?0x2388B3U:0x8BD3F7U;
    const uint32_t highlighted_arrow=theme==PANEL_THEME_LIGHT?0xFFFFFFU:0x071018U;
    const uint32_t normal=theme==PANEL_THEME_LIGHT?0x52616BU:0x80919EU;
    const size_t selected=lower?1U:0U,other=lower?0U:1U;
    lv_color_t fills_before[4],tracks_before[4];
    for(size_t line=0U;line<4U;line++){
        fills_before[line]=roller_fill_color(ui,widget,line);
        tracks_before[line]=lv_obj_get_style_bg_color(ui->roller_lines[widget][line],0);
    }
    roller_feedback_start(ui,widget,lower);
    assert(ui->roller_feedback_active&&ui->roller_feedback_timer);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][selected],0),lv_color_hex(highlighted_arrow)));
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][other],0),lv_color_hex(normal)));
    assert_color(ui->roller_arrow_highlights[widget][selected],highlight);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][selected],0)==LV_OPA_COVER);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][other],0)==LV_OPA_TRANSP);
    for(size_t line=0U;line<4U;line++){
        assert(lv_color_eq(roller_fill_color(ui,widget,line),fills_before[line]));
        assert(lv_color_eq(lv_obj_get_style_bg_color(ui->roller_lines[widget][line],0),tracks_before[line]));
    }
    lv_tick_inc(549U);lv_timer_handler();
    assert(ui->roller_feedback_active&&ui->roller_feedback_timer);
    lv_tick_inc(1U);lv_timer_handler();draw();
    assert(!ui->roller_feedback_active&&ui->roller_feedback_timer==NULL);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][selected],0),lv_color_hex(normal)));
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][selected],0)==LV_OPA_TRANSP);
    for(size_t line=0U;line<4U;line++){
        assert(lv_color_eq(roller_fill_color(ui,widget,line),fills_before[line]));
        assert(lv_color_eq(lv_obj_get_style_bg_color(ui->roller_lines[widget][line],0),tracks_before[line]));
    }
}
static void assert_roller_bar_geometry_and_palette(panel_ui_t *ui, uint8_t theme)
{
    lv_obj_update_layout(ui->screen);
    const uint32_t track=theme==PANEL_THEME_LIGHT?0xC4CDD3U:0x435562U;
    const uint32_t fill=theme==PANEL_THEME_LIGHT?0x2388B3U:0x8BD3F7U;
    for(size_t widget=1U;widget<=2U;widget++){
        const lv_obj_t *card=ui->widget_card[widget];
        lv_obj_t *upper_zone=lv_obj_get_parent(ui->roller_arrows[widget][0]);
        lv_obj_t *lower_zone=lv_obj_get_parent(ui->roller_arrows[widget][1]);
        assert(lv_obj_get_y(upper_zone)==24&&lv_obj_get_height(upper_zone)==44);
        assert(lv_obj_get_y(lower_zone)==116&&lv_obj_get_height(lower_zone)==44);
        assert(lv_obj_get_style_line_width(ui->roller_arrows[widget][0],0)==3);
        assert(lv_obj_get_style_line_width(ui->roller_arrows[widget][1],0)==3);
        for(size_t direction=0U;direction<2U;direction++){
            lv_obj_t *arrow=ui->roller_arrows[widget][direction];
            lv_obj_t *zone=lv_obj_get_parent(arrow);
            const lv_point_precise_t *points=lv_line_get_points(arrow);
            assert(points&&points[0].x==0&&points[1].x==10&&points[2].x==19);
            if(direction==0U) assert(points[0].y==11&&points[1].y==0&&points[2].y==11);
            else assert(points[0].y==0&&points[1].y==11&&points[2].y==0);
            /* The specified 20 x 12 is the chevron point geometry. LVGL's
             * line object bounds also include stroke/layout extents. */
            assert((points[2].x-points[0].x+1)==20);
            assert(((points[0].y > points[1].y
                         ? points[0].y-points[1].y
                         : points[1].y-points[0].y)+1)==12);
            /* Child coordinates are relative to the button's padded content
             * origin; include that origin when checking geometric centering. */
            assert(lv_obj_get_x(arrow)+lv_obj_get_style_pad_left(zone,0)+
                   lv_obj_get_width(arrow)/2==lv_obj_get_width(zone)/2);
            assert(lv_obj_get_y(arrow)+lv_obj_get_style_pad_top(zone,0)+
                   lv_obj_get_height(arrow)/2==lv_obj_get_height(zone)/2);
            assert(lv_obj_get_style_line_rounded(arrow,0));
            lv_obj_t *highlight=ui->roller_arrow_highlights[widget][direction];
            assert(lv_obj_get_width(highlight)==38&&lv_obj_get_height(highlight)==30);
            assert(lv_obj_get_x(highlight)+lv_obj_get_style_pad_left(zone,0)+
                   lv_obj_get_width(highlight)/2==lv_obj_get_width(zone)/2);
            assert(lv_obj_get_y(highlight)+lv_obj_get_style_pad_top(zone,0)+
                   lv_obj_get_height(highlight)/2==lv_obj_get_height(zone)/2);
            assert(lv_obj_get_style_radius(highlight,0)==6);
            assert(lv_obj_get_style_bg_opa(highlight,0)==LV_OPA_TRANSP);
        }
        int32_t center_sum=0;
        for(size_t line=0U;line<4U;line++){
            assert(lv_obj_get_width(ui->roller_lines[widget][line])==56);
            assert(lv_obj_get_height(ui->roller_lines[widget][line])==10);
            assert(lv_obj_get_width(ui->roller_fills[widget][line])==56);
            assert(lv_obj_get_height(ui->roller_fills[widget][line])==10);
            assert(lv_obj_get_y(ui->roller_lines[widget][line])==
                   lv_obj_get_y(ui->roller_fills[widget][line]));
            center_sum+=lv_obj_get_y(ui->roller_lines[widget][line])+5;
            assert_color(ui->roller_lines[widget][line],track);
            assert_color(ui->roller_fills[widget][line],fill);
            if(line>0U){
                assert(lv_obj_get_y(ui->roller_lines[widget][line])-
                       lv_obj_get_y(ui->roller_lines[widget][line-1U])==12);
            }
        }
        const int32_t arrow_midpoint=((lv_obj_get_y(upper_zone)+22)+
                                      (lv_obj_get_y(lower_zone)+22))/2;
        assert(arrow_midpoint==92);
        /* LVGL aligns children in the card's inner area; include the 1 px
         * border origin when comparing with the touch-zone card coordinates. */
        assert(center_sum/4+lv_obj_get_style_border_width((lv_obj_t *)card,0)==
               arrow_midpoint);
        assert(lv_obj_get_height((lv_obj_t *)card)==176);
    }
}
static void assert_feedback_theme_switch(panel_ui_t *ui)
{
    ui->model->settings.background_mode=PANEL_THEME_DARK;render_settings(ui);
    roller_feedback_start(ui,1U,false);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0x071018U)));
    assert_color(ui->roller_arrow_highlights[1][0],0x8BD3F7U);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_COVER);
    assert_color(ui->roller_fills[1][0],0x8BD3F7U);
    ui->model->settings.background_mode=PANEL_THEME_LIGHT;render_settings(ui);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0xFFFFFFU)));
    assert_color(ui->roller_arrow_highlights[1][0],0x2388B3U);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_COVER);
    assert_color(ui->roller_lines[1][0],0xC4CDD3U);
    assert_color(ui->roller_fills[1][0],0x2388B3U);
    assert_color(ui->roller_fills[1][2],0x2388B3U);
    lv_timer_t *timer=ui->roller_feedback_timer;assert(timer);
    roller_feedback_timer_cb(timer);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0x52616BU)));
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_TRANSP);
    assert_color(ui->roller_lines[1][0],0xC4CDD3U);
    assert_color(ui->roller_fills[1][0],0x2388B3U);
    ui->model->settings.background_mode=PANEL_THEME_DARK;render_settings(ui);
    assert_color(ui->roller_fills[1][0],0x8BD3F7U);
    roller_feedback_start(ui,1U,true);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][1],0),lv_color_hex(0x071018U)));
    assert_color(ui->roller_arrow_highlights[1][1],0x8BD3F7U);
    roller_feedback_cancel(ui);
    assert(ui->roller_feedback_timer==NULL&&!ui->roller_feedback_active);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][1],0),lv_color_hex(0x80919EU)));
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][1],0)==LV_OPA_TRANSP);
    assert_color(ui->roller_fills[1][0],0x8BD3F7U);
}
int main(void)
{
    lv_init();lv_display_t *d=lv_display_create(480,480);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,flush);
    panel_ui_model_t initial_light_model;panel_ui_model_init(&initial_light_model,0);
    initial_light_model.settings.background_mode=PANEL_THEME_LIGHT;
    panel_ui_t *initial_light_ui=NULL;
    panel_ui_config_t initial_light_cfg={.model=&initial_light_model};
    assert(panel_ui_create(&initial_light_ui,&initial_light_cfg));
    draw();
    assert_color(initial_light_ui->screen,0xF1F4F6);
    assert_roller_bar_geometry_and_palette(initial_light_ui,PANEL_THEME_LIGHT);
    panel_ui_destroy(&initial_light_ui);
    panel_ui_model_t m;panel_ui_model_init(&m,0);panel_ui_t *ui=NULL;
    panel_ui_config_t cfg={.model=&m,.callbacks={.read_information=read_info,
        .request_wifi_reconfigure=mutate,.request_wifi_cancel=mutate,
        .request_choose_homey=mutate,.request_homey_wipe=mutate,
        .request_change_athom_account=mutate,.settings_changed=settings}};
    assert(panel_ui_create(&ui,&cfg)&&panel_ui_activate(ui));draw();
    assert(m.settings.background_mode==PANEL_THEME_DARK);
    assert_theme_labels_valid(ui);
    assert_roller_bar_geometry_and_palette(ui,PANEL_THEME_DARK);
    assert_color(ui->screen,0x071018);assert_color(ui->widget_card[1],0x162735);
    for(size_t i=0;i<PANEL_UI_WIDGET_COUNT;i++)
        assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_card[i],0),lv_color_hex(0xE0E0E0)));
    assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_icon[4],0),lv_color_hex(0xF4F7FA)));
    assert(lv_color_eq(lv_obj_get_style_border_color(ui->confirmation_layer,0),lv_color_hex(0xD8E8F2)));
    for(size_t i=0;i<3U;i++){
        assert_color(ui->disabled_markis_controls[i],0x203746);
        assert(lv_color_eq(lv_obj_get_style_border_color(ui->disabled_markis_controls[i],0),lv_color_hex(0x506575)));
        assert(lv_color_eq(i==1U ? lv_obj_get_style_border_color(ui->disabled_markis_symbols[i],0)
                                 : lv_obj_get_style_line_color(ui->disabled_markis_symbols[i],0),
                            lv_color_hex(0xD9E2E8)));
        assert(lv_color_eq(lv_obj_get_style_text_color(ui->disabled_markis_captions[i],0),lv_color_hex(0xD9E2E8)));
    }
    assert(lv_color_eq(lv_obj_get_style_bg_color(ui->disabled_markis_stop_marks[1],0),lv_color_hex(0xD9E2E8)));
    assert(lv_obj_get_style_bg_grad_dir(ui->screen,0)==LV_GRAD_DIR_NONE);
    lv_obj_t *background_button=lv_obj_get_parent(ui->background_label);
    lv_color_t blue_button=lv_obj_get_style_bg_color(background_button,LV_PART_MAIN);
    lv_color_t white_button_text=lv_obj_get_style_text_color(ui->background_label,0);
    lv_obj_t *back_canvas=lv_obj_get_child(ui->settings_back_button,0);
    lv_obj_t *nav_icon=find_line(ui->settings_back_button);
    const lv_point_precise_t *back_points=lv_line_get_points(nav_icon);
    assert(back_canvas&&lv_obj_get_width(back_canvas)==11&&lv_obj_get_height(back_canvas)==17);
    assert(back_points&&back_points[0].x==10&&back_points[0].y==0&&
           back_points[1].x==0&&back_points[1].y==8&&
           back_points[2].x==10&&back_points[2].y==16);
    assert(lv_obj_get_style_line_width(nav_icon,0)==3&&lv_obj_get_style_line_rounded(nav_icon,0));
    lv_color_t white_button_icon=lv_obj_get_style_line_color(nav_icon,0);
    const int32_t button_width=lv_obj_get_width(background_button);
    const int32_t button_height=lv_obj_get_height(background_button);
    assert(lv_color_eq(white_button_text,lv_color_hex(0xF4F7FA)));
    assert(lv_color_eq(white_button_icon,lv_color_hex(0xF4F7FA)));
    assert(panel_ui_open_settings(ui));
    click(background_button);
    assert(m.settings.background_mode==PANEL_THEME_LIGHT&&settings_calls==1U);
    assert(last_saved_settings.background_mode==PANEL_THEME_LIGHT);
    assert_color(ui->screen,0xF1F4F6);assert_color(ui->settings_layer,0xE9EEF1);
    assert_roller_bar_geometry_and_palette(ui,PANEL_THEME_LIGHT);
    for(size_t i=0;i<PANEL_UI_WIDGET_COUNT;i++)assert_color(ui->widget_card[i],0xFFFFFF);
    for(size_t i=0;i<PANEL_UI_WIDGET_COUNT;i++)
        assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_card[i],0),lv_color_hex(0x263746)));
    assert_color(ui->status_dialog,0xFFFFFF);assert_color(ui->confirmation_layer,0xFFFFFF);
    assert_color(ui->widget_icon[4],0x596A78);
    assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_icon[4],0),lv_color_hex(0x263746)));
    for(size_t i=0;i<3U;i++)
        assert(lv_color_eq(i==1U ? lv_obj_get_style_border_color(ui->disabled_markis_symbols[i],0)
                                 : lv_obj_get_style_line_color(ui->disabled_markis_symbols[i],0),
                            lv_color_hex(0x111820)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(ui->disabled_markis_stop_marks[1],0),lv_color_hex(0x111820)));
    for(size_t i=0;i<3U;i++)
        assert(lv_color_eq(lv_obj_get_style_text_color(ui->disabled_markis_captions[i],0),lv_color_hex(0x111820)));
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->settings_header_title,0),lv_color_hex(0x111820)));
    assert(lv_color_eq(lv_obj_get_style_text_color(find_label(ui->settings_advanced_body,"SYSTEM"),0),lv_color_hex(0x005B80)));
    assert(lv_obj_get_style_bg_grad_dir(ui->screen,0)==LV_GRAD_DIR_NONE);
    assert(lv_color_eq(lv_obj_get_style_bg_color(background_button,LV_PART_MAIN),blue_button));
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->background_label,0),white_button_text));
    assert(lv_color_eq(lv_obj_get_style_line_color(nav_icon,0),white_button_icon));
    assert(lv_obj_get_width(background_button)==button_width&&lv_obj_get_height(background_button)==button_height);
    /* Confirmation and error surfaces use the active palette without recoloring blue controls. */
    panel_ui_request_confirmation(&m,PANEL_CONFIRM_HOMEY_WIPE);assert(panel_ui_refresh(ui));draw();
    assert(!lv_obj_has_flag(ui->confirmation_layer,LV_OBJ_FLAG_HIDDEN));
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->confirmation_text,0),lv_color_hex(0x111820)));
    panel_ui_cancel_confirmation(&m);assert(panel_ui_refresh(ui));
    panel_ui_request_confirmation(&m,PANEL_CONFIRM_HOMEY_WIPE);assert(panel_ui_refresh(ui));
    panel_ui_cancel_confirmation(&m);assert(panel_ui_refresh(ui));
    click(background_button);
    assert(m.settings.background_mode==PANEL_THEME_DARK&&settings_calls==2U);
    assert_color(ui->screen,0x071018);assert_color(ui->settings_layer,0x0D1B25);
    for(size_t i=0;i<PANEL_UI_WIDGET_COUNT;i++)
        assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_card[i],0),lv_color_hex(0xE0E0E0)));
    assert(lv_color_eq(lv_obj_get_style_border_color(ui->widget_icon[4],0),lv_color_hex(0xF4F7FA)));
    assert(lv_color_eq(lv_obj_get_style_border_color(ui->confirmation_layer,0),lv_color_hex(0xD8E8F2)));
    for(size_t i=0;i<3U;i++){
        assert_color(ui->disabled_markis_controls[i],0x203746);
        assert(lv_color_eq(lv_obj_get_style_border_color(ui->disabled_markis_controls[i],0),lv_color_hex(0x506575)));
        assert(lv_color_eq(i==1U ? lv_obj_get_style_border_color(ui->disabled_markis_symbols[i],0)
                                 : lv_obj_get_style_line_color(ui->disabled_markis_symbols[i],0),
                            lv_color_hex(0xD9E2E8)));
        assert(lv_color_eq(lv_obj_get_style_text_color(ui->disabled_markis_captions[i],0),lv_color_hex(0xD9E2E8)));
    }
    assert(lv_color_eq(lv_obj_get_style_bg_color(ui->disabled_markis_stop_marks[1],0),lv_color_hex(0xD9E2E8)));
    assert(lv_obj_get_style_bg_grad_dir(ui->screen,0)==LV_GRAD_DIR_NONE);
    assert(lv_color_eq(lv_obj_get_style_bg_color(background_button,LV_PART_MAIN),blue_button));
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->background_label,0),white_button_text));
    assert(lv_color_eq(lv_obj_get_style_line_color(nav_icon,0),white_button_icon));
    assert(lv_obj_get_width(background_button)==button_width&&lv_obj_get_height(background_button)==button_height);
    assert_theme_labels_valid(ui);
    assert_feedback_theme_switch(ui);
    assert_roller_bar_geometry_and_palette(ui,PANEL_THEME_DARK);
    click(background_button);assert(m.settings.background_mode==PANEL_THEME_LIGHT);
    click(button(ui->settings_main_body,"Avancerat"));
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
        assert(lv_color_eq(lv_obj_get_style_text_color(ui->status_text,0),lv_color_hex(0x111820)));
        assert_color(ui->status_dialog,0xFFFFFF);
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
    assert_color(ui->status_dialog,0xFFFFFF);
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->wifi_ssid,0),lv_color_hex(0x111820)));
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
    assert(confirmed.background_mode==PANEL_THEME_LIGHT);
    click(background_button);assert(m.settings.background_mode==PANEL_THEME_DARK);
    assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_SAVING));
    assert(panel_ui_restore_settings(ui,&confirmed));
    assert(m.settings.background_mode==PANEL_THEME_LIGHT);assert_color(ui->screen,0xF1F4F6);
    assert(panel_ui_set_settings_save_status(ui,PANEL_UI_SETTINGS_SAVE_FAILED));
    assert(ui->modal_kind==MODAL_ERROR&&lv_color_eq(
        lv_obj_get_style_text_color(ui->status_text,0),lv_color_hex(0x111820)));
    assert_color(ui->status_dialog,0xFFFFFF);click(ui->status_close_footer);
    lv_obj_t *brightness=lv_obj_get_parent(ui->normal_brightness_label);
    lv_color_t normal=lv_obj_get_style_bg_color(brightness,LV_PART_MAIN);
    lv_obj_add_state(brightness,LV_STATE_PRESSED);draw();
    assert(!lv_color_eq(normal,lv_obj_get_style_bg_color(brightness,LV_PART_MAIN)));
    lv_obj_remove_state(brightness,LV_STATE_PRESSED);lv_tick_inc(300);draw();
    assert(lv_color_eq(normal,lv_obj_get_style_bg_color(brightness,LV_PART_MAIN)));
    click(brightness);assert(settings_calls==5U&&m.settings.normal_brightness!=confirmed.normal_brightness);
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
    ui->homey_data_ready=true;m.homey_snapshot_stale=false;
    for(size_t widget=1;widget<=2;widget++){
        m.widget_status[widget]=PANEL_WIDGET_AVAILABLE;
        m.roller_position_valid[widget]=true;m.roller_position_step[widget]=2U;
    }
    assert(panel_ui_refresh(ui));draw();
    assert_single_arrow_feedback(ui,1U,false,PANEL_THEME_LIGHT);
    assert_single_arrow_feedback(ui,2U,true,PANEL_THEME_DARK);
    lv_mem_monitor_t feedback_before,feedback_after;lv_mem_monitor(&feedback_before);
    for(unsigned n=0U;n<100U;n++){
        roller_feedback_start(ui,1U,(n&1U)!=0U);
        roller_feedback_start(ui,2U,(n&1U)==0U);
        roller_feedback_cancel(ui);
    }
    lv_mem_monitor(&feedback_after);assert(feedback_after.free_size==feedback_before.free_size);
    assert(mutations==0);
    assert_theme_labels_valid(ui);
    printf("PATCH087_REAL_LVGL_FIVE_VIEWS_REFRESH_QR_LIFECYCLE PASS frames=%u cycles=100 pool_delta=%ld\n",frames,(long)after.free_size-(long)before.free_size);
    roller_feedback_start(ui,1U,false);
    panel_ui_destroy(&ui);assert(!ui);lv_display_delete(d);lv_deinit();
}
