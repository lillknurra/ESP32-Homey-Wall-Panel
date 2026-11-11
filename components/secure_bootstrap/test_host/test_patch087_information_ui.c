#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "panel_homey_favorites.h"
#include "lvgl.h"
#include "homey_panel_font_16.h"
bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *m){(void)m;return false;}
bool panel_homey_favorites_light_toggle_execution_ready(size_t i,bool r){(void)i;(void)r;return false;}
#include "../panel_ui.c"
static uint8_t pixels[480*40*2];
static uint8_t framebuffer[480*480*2];
static unsigned reads,mutations,frames;
static panel_information_kind_t last_information_kind;
static unsigned settings_calls;
static unsigned commission_reviews,commission_requests,commission_cancels;
static panel_ui_settings_t last_saved_settings;
static bool present=true;
static bool commission_review_available=true,commission_request_queued=true;
static panel_information_t fixture;
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *p)
{
    unsigned w=(unsigned)(a->x2-a->x1+1);
    for(int y=a->y1;y<=a->y2;y++) memcpy(framebuffer+2*(y*480+a->x1),p+2*(y-a->y1)*w,2*w);
    frames++;lv_display_flush_ready(d);
}
static bool read_info(void *ctx,panel_information_kind_t k,panel_information_t *i)
{(void)ctx;last_information_kind=k;reads++;*i=fixture;return present;}
static void mutate(void *p){(void)p;mutations++;}
static panel_ui_roller_request_result_t unexpected_awning_request(
    void *p,size_t w,int8_t d,uint8_t *m,uint8_t *q,bool *f)
{(void)p;(void)w;(void)d;(void)m;(void)q;(void)f;mutations++;return PANEL_UI_ROLLER_NOT_READY;}

static void settings(void *p,const panel_ui_settings_t *s)
{(void)p;assert(s);last_saved_settings=*s;settings_calls++;}
static bool commission_review(void *p,char *out,size_t capacity)
{
    (void)p;commission_reviews++;
    int n=snprintf(out,capacity,"Homey READY - färsk snapshot 7\nSolskydd 53%% - Balkongmarkis 41%%");
    return commission_review_available&&n>=0&&(size_t)n<capacity;
}
static bool commission_confirm(void *p)
{(void)p;commission_requests++;return commission_request_queued;}
static void commission_cancel(void *p){(void)p;commission_cancels++;}
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
static bool areas_overlap(lv_obj_t *a,lv_obj_t *b)
{
    lv_area_t aa,bb;lv_obj_get_coords(a,&aa);lv_obj_get_coords(b,&bb);
    return aa.x1<=bb.x2&&bb.x1<=aa.x2&&aa.y1<=bb.y2&&bb.y1<=aa.y2;
}
static void assert_visible_bar_group_centered(panel_ui_t *ui,size_t widget)
{
    lv_obj_t *bar=ui->roller_lines[widget][0];
    const bool was_hidden=lv_obj_is_hidden(bar);
    lv_obj_set_hidden(bar,false);
    lv_obj_update_layout(ui->screen);
    lv_area_t bar_area,upper_zone_area,lower_zone_area;
    lv_obj_get_coords(bar,&bar_area);
    lv_obj_get_coords(lv_obj_get_parent(ui->roller_arrows[widget][0]),&upper_zone_area);
    lv_obj_get_coords(lv_obj_get_parent(ui->roller_arrows[widget][1]),&lower_zone_area);
    const int32_t bar_center_twice=bar_area.x1+bar_area.x2;
    const int32_t upper_arrow_center_twice=upper_zone_area.x1+upper_zone_area.x2;
    const int32_t lower_arrow_center_twice=lower_zone_area.x1+lower_zone_area.x2;
    fprintf(stderr,"P4J geometry widget=%zu bar_center_x2=%d upper_arrow_center_x2=%d lower_arrow_center_x2=%d delta_px=%d\n",
            widget,bar_center_twice,upper_arrow_center_twice,lower_arrow_center_twice,
            (bar_center_twice-upper_arrow_center_twice)/2);
    assert(abs(bar_center_twice-upper_arrow_center_twice)<=2);
    assert(abs(bar_center_twice-lower_arrow_center_twice)<=2);
    for(size_t line=0U;line<4U;line++){
        lv_area_t line_area,fill_area,pending_area;
        lv_obj_get_coords(ui->roller_lines[widget][line],&line_area);
        lv_obj_get_coords(ui->roller_fills[widget][line],&fill_area);
        lv_obj_get_coords(ui->roller_pending_fills[widget][line],&pending_area);
        assert(abs((line_area.x1+line_area.x2)-bar_center_twice)<=2);
        assert(abs((fill_area.x1+fill_area.x2)-bar_center_twice)<=2);
        assert(abs((pending_area.x1+pending_area.x2)-bar_center_twice)<=2);
    }
    if(was_hidden)lv_obj_set_hidden(bar,true);
}
static void assert_status_copy_fits(lv_obj_t *label)
{
    lv_point_t size={0};
    const lv_font_t *font=lv_obj_get_style_text_font(label,0);
    lv_text_get_size(&size,lv_label_get_text(label),font,
        lv_obj_get_style_text_letter_space(label,0),
        lv_obj_get_style_text_line_space(label,0),lv_obj_get_width(label),
        LV_TEXT_FLAG_NONE);
    assert(size.x<=lv_obj_get_width(label)&&size.y<=lv_obj_get_height(label));
}
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
    assert(ROLLER_REJECTION_FEEDBACK_MS==1200U);
    ui->model->settings.background_mode=theme;render_settings(ui);
    const uint32_t normal=theme==PANEL_THEME_LIGHT?0x52616BU:0x80919EU;
    const size_t selected=lower?1U:0U,other=lower?0U:1U;
    lv_color_t fills_before[4],tracks_before[4];
    for(size_t line=0U;line<4U;line++){
        fills_before[line]=roller_fill_color(ui,widget,line);
        tracks_before[line]=lv_obj_get_style_bg_color(ui->roller_lines[widget][line],0);
    }
    roller_feedback_reject(ui,widget,lower);
    assert(ui->roller_feedback_active&&ui->roller_feedback_timer);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][selected],0),lv_color_hex(0xD62828U)));
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[widget][other],0),lv_color_hex(normal)));
    assert_color(ui->roller_arrow_highlights[widget][selected],0xD62828U);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][selected],0)==LV_OPA_COVER);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[widget][other],0)==LV_OPA_TRANSP);
    for(size_t line=0U;line<4U;line++){
        assert(lv_color_eq(roller_fill_color(ui,widget,line),fills_before[line]));
        assert(lv_color_eq(lv_obj_get_style_bg_color(ui->roller_lines[widget][line],0),tracks_before[line]));
    }
    roller_feedback_cancel(ui);draw();
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
        assert(!areas_overlap(ui->widget_status[widget],ui->widget_title[widget]));
        for(size_t line=0U;line<4U;line++)
            assert(lv_obj_is_hidden(ui->widget_status[widget]) ||
                   lv_obj_is_hidden(ui->roller_lines[widget][line]));
        for(size_t direction=0U;direction<2U;direction++)
            assert(!areas_overlap(ui->widget_status[widget],ui->roller_arrows[widget][direction]));
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
        for(size_t line=0U;line<4U;line++){
            assert(lv_obj_get_width(ui->roller_lines[widget][line])==56);
            assert(lv_obj_get_height(ui->roller_lines[widget][line])==10);
            assert(lv_obj_get_width(ui->roller_fills[widget][line])==56);
            assert(lv_obj_get_height(ui->roller_fills[widget][line])==10);
            assert(lv_obj_get_y(ui->roller_lines[widget][line])==
                   lv_obj_get_y(ui->roller_fills[widget][line]));
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
        assert(lv_obj_get_y(ui->roller_lines[widget][0])==70);
        assert(lv_obj_get_y(ui->roller_lines[widget][3])+10<=116);
        lv_area_t status_area,card_area;
        lv_obj_get_coords(ui->widget_status[widget],&status_area);
        lv_obj_get_coords((lv_obj_t *)card,&card_area);
        assert(status_area.x1>=card_area.x1+2&&status_area.x2<=card_area.x2-2);
        assert(status_area.y1>=card_area.y1+2&&status_area.y2<=card_area.y2-2);
        assert(lv_obj_get_width(ui->widget_status[widget])==ROLLER_CARD_W-12);
        assert(lv_obj_get_height(ui->widget_status[widget])==homey_panel_font_16.line_height);
        assert(lv_obj_get_style_text_font(ui->widget_status[widget],0)==&homey_panel_font_16);
        assert(lv_color_eq(lv_obj_get_style_text_color(ui->widget_status[widget],0),
            lv_color_hex(theme==PANEL_THEME_LIGHT?0x111820U:0xF4F7FAU)));
        assert(lv_obj_get_height((lv_obj_t *)card)==176);
    }
}
static void assert_roller_idle(panel_ui_t *ui,size_t widget,
                               roller_presentation_t expected,const char *text)
{
    assert(roller_presentation(ui,widget)==expected);
    const panel_ui_model_t model_before=*ui->model;
    const panel_ui_awning_guard_state_t guard_before=ui->roller_guard_state[widget];
    render_roller_widget(ui,widget);lv_obj_update_layout(ui->screen);
    assert(memcmp(&model_before,ui->model,sizeof(model_before))==0);
    assert(ui->roller_guard_state[widget]==guard_before);
    const bool bars=expected==ROLLER_PRESENTATION_POSITION;
    assert(lv_obj_is_hidden(ui->widget_status[widget])==bars);
    if(bars)assert_visible_bar_group_centered(ui,widget);
    if(!bars){
        assert(strcmp(lv_label_get_text(ui->widget_status[widget]),text)==0);
        assert_status_copy_fits(ui->widget_status[widget]);
        lv_area_t label,upper,lower,card;
        lv_obj_get_coords(ui->widget_status[widget],&label);
        lv_obj_get_coords(ui->widget_card[widget],&card);
        lv_obj_get_coords(lv_obj_get_parent(ui->roller_arrows[widget][0]),&upper);
        lv_obj_get_coords(lv_obj_get_parent(ui->roller_arrows[widget][1]),&lower);
        fprintf(stderr,"P4I bounds widget=%zu label=(%d,%d,%d,%d) card=(%d,%d,%d,%d) gap=%d..%d\n",widget,label.x1,label.y1,label.x2,label.y2,card.x1,card.y1,card.x2,card.y2,upper.y2,lower.y1);
        assert(abs((label.x1+label.x2)-(card.x1+card.x2))<=1);
        assert(abs((label.y1+label.y2)-(upper.y2+lower.y1))<=1);
        assert(label.x1>=card.x1&&label.x2<=card.x2);
        assert(label.y1>upper.y2&&label.y2<lower.y1);
        assert(!areas_overlap(ui->widget_status[widget],ui->roller_arrows[widget][0]));
        assert(!areas_overlap(ui->widget_status[widget],ui->roller_arrows[widget][1]));
        assert(lv_obj_get_style_text_align(ui->widget_status[widget],0)==LV_TEXT_ALIGN_CENTER);
        assert(lv_label_get_long_mode(ui->widget_status[widget])==LV_LABEL_LONG_CLIP);
    }
    for(size_t line=0;line<4;line++){
        assert(lv_obj_is_hidden(ui->roller_lines[widget][line])==!bars);
        assert(lv_obj_is_hidden(ui->roller_pending_fills[widget][line]));
        if(!bars) assert(lv_obj_is_hidden(ui->roller_fills[widget][line]));
    }
}
static void assert_roller_status_copy(panel_ui_t *ui)
{
    const panel_ui_model_t original_model=*ui->model;
    const bool original_ready=ui->homey_data_ready;
    panel_ui_set_view(ui->model,PANEL_UI_VIEW_DASHBOARD);render_view(ui);
    assert(panel_ui_select_page(ui,0U,false));
    assert(lv_obj_is_hidden(ui->settings_layer));
    panel_ui_awning_guard_state_t guards[3];
    uint8_t masks[3],queued[3];bool failed[3];int8_t directions[3];
    const unsigned old_mutations=mutations,old_requests=commission_requests;
    const unsigned old_reviews=commission_reviews,old_settings=settings_calls;
    lv_font_glyph_dsc_t glyph;
    const uint32_t swedish[]={0x00C5U,0x00E4U,0x00F6U};
    for(size_t n=0;n<3;n++)assert(lv_font_get_glyph_dsc(&homey_panel_font_16,&glyph,swedish[n],0));
    for(size_t w=1;w<=2;w++){
        guards[w]=ui->roller_guard_state[w];masks[w]=ui->roller_pending_bar_mask[w];
        queued[w]=ui->roller_queued_count[w];failed[w]=ui->roller_interaction_failed[w];
        directions[w]=ui->roller_active_direction[w];
    }
    for(unsigned theme=PANEL_THEME_DARK;theme<=PANEL_THEME_LIGHT;theme++){
        ui->model->settings.background_mode=theme;theme_apply(ui);
        for(size_t w=1;w<=2;w++){
            ui->homey_data_ready=true;ui->model->homey_snapshot_stale=false;
            ui->model->widget_status[w]=PANEL_WIDGET_AVAILABLE;
            ui->model->roller_position_valid[w]=true;ui->model->roller_position_step[w]=2;
            ui->model->roller_last_confirmed_position_valid[w]=true;
            ui->model->roller_last_confirmed_position_step[w]=2;
            ui->roller_active_direction[w]=0;ui->roller_pending_bar_mask[w]=0;
            ui->roller_queued_count[w]=0;ui->roller_interaction_failed[w]=false;
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_READY;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_POSITION,NULL);
            for(uint8_t step=0U;step<=PANEL_ROLLER_MAX_STEP;step+=PANEL_ROLLER_MAX_STEP){
                ui->model->roller_position_step[w]=step;
                assert_roller_idle(ui,w,ROLLER_PRESENTATION_POSITION,NULL);
            }
            ui->model->roller_position_step[w]=2U;
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_WRITE_PENDING;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_POSITION,NULL);
            ui->model->roller_position_valid[w]=false;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_READY;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->model->roller_position_valid[w]=true;
            ui->model->roller_position_step[w]=255;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->model->roller_position_step[w]=2;
            ui->model->homey_snapshot_stale=true;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->model->widget_status[w]=PANEL_WIDGET_UNAVAILABLE;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->model->homey_snapshot_stale=false;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNAVAILABLE,"Otillgänglig");
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_QUARANTINED;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNAVAILABLE,"Otillgänglig");
            ui->model->widget_status[w]=PANEL_WIDGET_AVAILABLE;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_ACTION_REQUIRED,"Åtgärd krävs");
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_UNINITIALIZED;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_ACTION_REQUIRED,"Åtgärd krävs");
            panel_ui_set_roller_guard_state(ui,w,(panel_ui_awning_guard_state_t)255);
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_READY;
            ui->homey_data_ready=false;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->homey_data_ready=true;ui->model->widget_status[w]=PANEL_WIDGET_UNKNOWN;
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNKNOWN,"Okänd");
            ui->model->widget_status[w]=PANEL_WIDGET_AVAILABLE;
            /* Queued intention without dispatched direction never blinks. */
            panel_ui_set_roller_interaction(ui,w,2,1,false);
            assert_roller_idle(ui,w,ROLLER_PRESENTATION_POSITION,NULL);
            for(int direction=-1;direction<=1;direction+=2){
                panel_ui_set_roller_active_direction(ui,w,(int8_t)direction);
                assert(roller_feedback_has_pending(ui));
                for(unsigned degraded=0;degraded<4;degraded++){
                    ui->model->homey_snapshot_stale=degraded==1;
                    ui->model->roller_position_valid[w]=degraded<2;
                    ui->model->widget_status[w]=degraded==3?PANEL_WIDGET_UNAVAILABLE:PANEL_WIDGET_AVAILABLE;
                    render_roller_widget(ui,w);
                    assert(roller_presentation(ui,w)==(direction<0?ROLLER_PRESENTATION_MOVING_UP:ROLLER_PRESENTATION_MOVING_DOWN));
                    assert(lv_obj_is_hidden(ui->widget_status[w]));
                    assert(!lv_obj_is_hidden(ui->roller_lines[w][0]));
                    assert(!lv_obj_is_hidden(ui->roller_fills[w][0]));
                    assert(!lv_obj_is_hidden(ui->roller_fills[w][1]));
                    assert_visible_bar_group_centered(ui,w);
                    /* Actual LVGL timer over multiple frames: no fallback flicker. */
                    for(unsigned frame=0;frame<4;frame++){
                        const bool before=lv_obj_is_hidden(ui->roller_pending_fills[w][1]);
                        lv_tick_inc(ROLLER_FEEDBACK_BLINK_MS);lv_timer_handler();lv_refr_now(NULL);
                        assert(lv_obj_is_hidden(ui->roller_pending_fills[w][1])!=before);
                        assert(lv_obj_is_hidden(ui->widget_status[w]));
                    }
                }
                /* Terminal failure/timeout wins by ending authoritative activity. */
                panel_ui_set_roller_interaction(ui,w,0,0,true);
                panel_ui_set_roller_active_direction(ui,w,0);
                ui->model->homey_snapshot_stale=false;
                assert_roller_idle(ui,w,ROLLER_PRESENTATION_UNAVAILABLE,"Otillgänglig");
                ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_QUARANTINED;
                ui->model->widget_status[w]=PANEL_WIDGET_AVAILABLE;
                assert_roller_idle(ui,w,ROLLER_PRESENTATION_ACTION_REQUIRED,"Åtgärd krävs");
                ui->roller_guard_state[w]=PANEL_UI_AWNING_GUARD_READY;
                ui->model->roller_position_valid[w]=true;
                ui->roller_interaction_failed[w]=false;ui->roller_pending_bar_mask[w]=2;
                assert_roller_idle(ui,w,ROLLER_PRESENTATION_POSITION,NULL);
            }
            ui->roller_pending_bar_mask[w]=0;ui->roller_active_direction[w]=0;
        }
        /* ON/OFF screenshots are separated by the real 650 ms LVGL timer. */
        for(unsigned scenario=0;scenario<6;scenario++){
            for(size_t w=1;w<=2;w++){
                ui->model->homey_snapshot_stale=false;ui->model->widget_status[w]=scenario==3?PANEL_WIDGET_UNAVAILABLE:PANEL_WIDGET_AVAILABLE;
                ui->model->roller_position_valid[w]=scenario!=2;ui->model->roller_position_step[w]=2;
                ui->roller_guard_state[w]=scenario==1?PANEL_UI_AWNING_GUARD_UNINITIALIZED:PANEL_UI_AWNING_GUARD_READY;
                ui->roller_interaction_failed[w]=false;ui->roller_pending_bar_mask[w]=scenario==5?4:(scenario==4?2:0);
                panel_ui_set_roller_active_direction(ui,w,scenario==5?1:(scenario==4?-1:0));
                render_roller_widget(ui,w);
            }
            if(scenario>=4){
                assert(ui->roller_feedback_timer);lv_timer_reset(ui->roller_feedback_timer);
            }
            draw();capture(20+theme*8+(scenario<4?scenario:(scenario==4?4:6)));
            if(scenario>=4){
                const size_t line=scenario==4?1:2;
                for(size_t w=1;w<=2;w++)assert(!lv_obj_is_hidden(ui->roller_pending_fills[w][line]));
                lv_tick_inc(ROLLER_FEEDBACK_BLINK_MS);lv_timer_handler();lv_refr_now(NULL);
                for(size_t w=1;w<=2;w++){
                    assert(lv_obj_is_hidden(ui->roller_pending_fills[w][line]));
                    assert(lv_obj_is_hidden(ui->widget_status[w]));
                }
                capture(20+theme*8+(scenario==4?5:7));
            }
        }
    }
    *ui->model=original_model;ui->homey_data_ready=original_ready;theme_apply(ui);
    render_view(ui);lv_obj_scroll_to_x(ui->pager,(int32_t)original_model.active_page*W,LV_ANIM_OFF);
    for(size_t w=1;w<=2;w++){
        ui->roller_guard_state[w]=guards[w];ui->roller_pending_bar_mask[w]=masks[w];
        ui->roller_queued_count[w]=queued[w];ui->roller_interaction_failed[w]=failed[w];
        ui->roller_active_direction[w]=directions[w];render_roller_widget(ui,w);
    }
    roller_feedback_cancel(ui);
    assert(mutations==old_mutations&&commission_requests==old_requests&&
           commission_reviews==old_reviews&&settings_calls==old_settings);
    puts("PATCH119P4I_PRESENTATION_MOVEMENT_GEOMETRY_THEMES_NO_WRITES PASS");
}
static void assert_feedback_theme_switch(panel_ui_t *ui)
{
    ui->model->settings.background_mode=PANEL_THEME_DARK;render_settings(ui);
    roller_feedback_reject(ui,1U,false);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0xD62828U)));
    assert_color(ui->roller_arrow_highlights[1][0],0xD62828U);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_COVER);
    assert_color(ui->roller_fills[1][0],0x8BD3F7U);
    ui->model->settings.background_mode=PANEL_THEME_LIGHT;render_settings(ui);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0xD62828U)));
    assert_color(ui->roller_arrow_highlights[1][0],0xD62828U);
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_COVER);
    assert_color(ui->roller_lines[1][0],0xC4CDD3U);
    assert_color(ui->roller_fills[1][0],0x2388B3U);
    assert_color(ui->roller_fills[1][2],0x2388B3U);
    assert(ui->roller_feedback_timer);
    roller_feedback_cancel(ui);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][0],0),lv_color_hex(0x52616BU)));
    assert(lv_obj_get_style_bg_opa(ui->roller_arrow_highlights[1][0],0)==LV_OPA_TRANSP);
    assert_color(ui->roller_lines[1][0],0xC4CDD3U);
    assert_color(ui->roller_fills[1][0],0x2388B3U);
    ui->model->settings.background_mode=PANEL_THEME_DARK;render_settings(ui);
    assert_color(ui->roller_fills[1][0],0x8BD3F7U);
    roller_feedback_reject(ui,1U,true);
    assert(lv_color_eq(lv_obj_get_style_line_color(ui->roller_arrows[1][1],0),lv_color_hex(0xD62828U)));
    assert_color(ui->roller_arrow_highlights[1][1],0xD62828U);
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
    for(size_t widget=1U;widget<=2U;widget++)assert_visible_bar_group_centered(initial_light_ui,widget);
    panel_ui_destroy(&initial_light_ui);
    panel_ui_model_t m;panel_ui_model_init(&m,0);panel_ui_t *ui=NULL;
    panel_ui_config_t cfg={.model=&m,.callbacks={.read_information=read_info,
        .request_wifi_reconfigure=mutate,.request_wifi_cancel=mutate,
        .request_choose_homey=mutate,.request_homey_wipe=mutate,
        .request_change_athom_account=mutate,.settings_changed=settings,
        .request_roller_step=unexpected_awning_request,
        .review_awning_commission=commission_review,
        .confirm_awning_commission=commission_confirm,
        .cancel_awning_commission_review=commission_cancel}};
    assert(panel_ui_create(&ui,&cfg)&&panel_ui_activate(ui));draw();
    capture(10);
    assert(commission_reviews==0U&&commission_requests==0U&&commission_cancels==0U);
    assert(m.settings.background_mode==PANEL_THEME_DARK);
    assert_theme_labels_valid(ui);
    assert_roller_bar_geometry_and_palette(ui,PANEL_THEME_DARK);
    panel_ui_model_t saved_dashboard_model=*ui->model;
    const bool saved_dashboard_ready=ui->homey_data_ready;
    ui->homey_data_ready=true;
    ui->model->homey_snapshot_stale=false;
    for(size_t widget=1U;widget<=2U;widget++){
        ui->model->widget_status[widget]=PANEL_WIDGET_AVAILABLE;
        ui->model->roller_position_valid[widget]=true;
        ui->model->roller_position_step[widget]=(uint8_t)(widget+1U);
        ui->roller_guard_state[widget]=PANEL_UI_AWNING_GUARD_UNINITIALIZED;
        render_roller_widget(ui,widget);
        assert(strcmp(lv_label_get_text(ui->widget_status[widget]),"Åtgärd krävs")==0);
        assert(lv_obj_is_hidden(ui->roller_lines[widget][0]));
        assert(lv_obj_is_hidden(ui->roller_fills[widget][0]));
    }
    draw();capture(11);
    *ui->model=saved_dashboard_model;
    ui->homey_data_ready=saved_dashboard_ready;
    render_roller_widget(ui,1U);render_roller_widget(ui,2U);draw();
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
    assert_roller_status_copy(ui);
    click(background_button);assert(m.settings.background_mode==PANEL_THEME_LIGHT);
    click(button(ui->settings_main_body,"Avancerat"));
    unsigned reviews_before=commission_reviews,requests_before=commission_requests;
    assert(panel_ui_refresh(ui));draw();
    assert(commission_reviews==reviews_before&&commission_requests==requests_before);
    click(button(ui->settings_advanced_body,"Granska första driftsättning"));
    assert(commission_reviews==reviews_before+1U&&commission_requests==requests_before);
    assert(ui->modal_kind==MODAL_HELP&&!lv_obj_has_flag(ui->status_overlay,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_width(ui->status_dialog)==416&&lv_obj_get_height(ui->status_dialog)==420);
    assert(lv_obj_get_width(ui->status_body)==384&&lv_obj_get_height(ui->status_body)==270);
    assert(lv_obj_get_scroll_dir(ui->status_body)==LV_DIR_NONE);
    assert(lv_obj_get_scroll_bottom(ui->status_body)<=0);
    assert(lv_obj_get_height(ui->status_text)<=lv_obj_get_height(ui->status_body));
    assert(lv_obj_get_style_text_font(ui->status_text,0)==&homey_panel_font_16);
    assert(strstr(lv_label_get_text(ui->status_text),"Homey READY"));
    assert(strstr(lv_label_get_text(ui->status_text),"färska och identitetsbundna"));
    assert(strstr(lv_label_get_text(ui->status_text),"Position ensam"));
    assert(strstr(lv_label_get_text(ui->status_text),"Kontrollerad migrering"));
    assert(strstr(lv_label_get_text(ui->status_text),"radering"));
    assert(strstr(lv_label_get_text(ui->status_text),"WRITE_PENDING"));
    assert(!strstr(lv_label_get_text(ui->status_text),"\xE2\x80\x93"));
    assert(!strstr(lv_label_get_text(ui->status_text),"\xC2\xB7"));
    lv_obj_t *action_label=lv_obj_get_child(ui->commission_action,0);
    assert(action_label&&strcmp(lv_label_get_text(action_label),"Till bekräftelse")==0);
    assert_status_copy_fits(action_label);
    assert_status_copy_fits(lv_obj_get_child(ui->status_close_footer,0));
    assert(!areas_overlap(ui->status_close_footer,ui->commission_action));
    lv_area_t dialog_area,action_area;
    lv_obj_get_coords(ui->status_dialog,&dialog_area);
    lv_obj_get_coords(ui->commission_action,&action_area);
    assert(action_area.x1>=dialog_area.x1&&action_area.x2<=dialog_area.x2);
    assert(action_area.y1>=dialog_area.y1&&action_area.y2<=dialog_area.y2);
    capture(6);
    click(ui->commission_action);
    assert(ui->commission_stage==2U&&commission_requests==requests_before);
    assert(strcmp(lv_label_get_text(ui->status_title),"Bekräfta aktivering")==0);
    assert(strstr(lv_label_get_text(ui->status_text),"granskat kontrollerad migrering"));
    assert(strstr(lv_label_get_text(ui->status_text),"Saknad NVS-post"));
    assert(strstr(lv_label_get_text(ui->status_text),"WRITE_PENDING/karantän"));
    assert(lv_obj_get_scroll_bottom(ui->status_body)<=0&&
           lv_obj_get_height(ui->status_text)<=lv_obj_get_height(ui->status_body));
    action_label=lv_obj_get_child(ui->commission_action,0);
    assert(action_label&&strcmp(lv_label_get_text(action_label),"Bekräfta")==0);
    assert_status_copy_fits(action_label);
    capture(7);
    commission_request_queued=false;
    click(ui->commission_action);
    assert(commission_requests==requests_before+1U&&commission_cancels==1U);
    assert(strcmp(lv_label_get_text(ui->status_title),"Aktivering nekad")==0);
    assert(strstr(lv_label_get_text(ui->status_text),"Ingen begäran köades"));
    assert(strstr(lv_label_get_text(ui->status_text),"inte skyddets aktuella tillstånd"));
    assert(!strstr(lv_label_get_text(ui->status_text),"oförändrade"));
    assert(ui->roller_guard_state[1U]==PANEL_UI_AWNING_GUARD_UNINITIALIZED&&
           ui->roller_guard_state[2U]==PANEL_UI_AWNING_GUARD_UNINITIALIZED);
    assert(mutations==0U);
    capture(8);click(ui->status_close);
    commission_review_available=false;
    reviews_before=commission_reviews;
    click(button(ui->settings_advanced_body,"Granska första driftsättning"));
    assert(commission_reviews==reviews_before+1U&&
           commission_requests==requests_before+1U);
    assert(strcmp(lv_label_get_text(ui->status_title),"Service ej tillgänglig")==0);
    click(ui->status_close);commission_review_available=true;commission_request_queued=true;
    click(button(ui->settings_advanced_body,"Granska första driftsättning"));
    assert(ui->commission_stage==1U);
    click(ui->commission_action);assert(ui->commission_stage==2U);
    click(ui->commission_action);
    assert(commission_requests==requests_before+2U&&ui->commission_request_pending);
    assert(strcmp(lv_label_get_text(ui->status_title),"Aktivering begärd")==0);
    assert(strstr(lv_label_get_text(ui->status_text),"Begäran är köad"));
    assert(strstr(lv_label_get_text(ui->status_text),"blockerade tills READY"));
    panel_ui_set_roller_guard_state(ui,1U,PANEL_UI_AWNING_GUARD_READY);
    assert(strcmp(lv_label_get_text(ui->status_title),"Aktivering begärd")==0);
    panel_ui_set_roller_guard_state(ui,2U,PANEL_UI_AWNING_GUARD_READY);
    assert(!ui->commission_request_pending);
    assert(strcmp(lv_label_get_text(ui->status_title),"Aktivering verifierad")==0);
    assert(strstr(lv_label_get_text(ui->status_text),"sparats och verifierats"));
    assert(mutations==0U&&commission_requests==requests_before+2U);
    capture(9);click(ui->status_close);
    const char *labels[]={"Wi-Fi-status","Homey-anslutning · Inte redo","Firmwareversion","Tidssynkronisering","Systemdiagnostik","RGB-diagnostik"};
    fixture.wifi_known=fixture.provisioning_known=fixture.homey_known=true;
    fixture.homey_state=2;fixture.wifi_online=true;fixture.now_ms=5000;
    fixture.firmware_known=true;strcpy(fixture.version,"fixture-Patch087");strcpy(fixture.idf_version,"v6.0.1");
    strcpy(fixture.build_date,"Oct 17 2026");strcpy(fixture.build_time,"12:00:00");
    fixture.time_known=fixture.time_synchronized=fixture.swedish_timezone=true;
    fixture.memory_known=true;fixture.internal_free=24000;fixture.internal_minimum=1500;fixture.psram_free=6000000;
    fixture.rgb_known=true;fixture.rgb.observed_at_us=123000000U;
    fixture.rgb.eof=128;fixture.rgb.vsync=5;fixture.rgb.shortfall=1;
    fixture.rgb.startup_shortfall=0;fixture.rgb.restart_decisions=1;fixture.rgb.restart_paths=1;
    fixture.rgb.last_actual=20;fixture.rgb.expected=24;fixture.rgb.vsync_min_us=16000;
    fixture.rgb.vsync_max_us=17000;fixture.rgb.bounce_max_us=900;
    panel_information_observe_snapshot(&fixture.snapshot,PANEL_HOMEY_READ_OK,4000,2);
    for(unsigned k=0;k<6;k++){
        unsigned before=reads;click(button(ui->settings_advanced_body,labels[k]));
        assert(ui->modal_kind==MODAL_INFORMATION&&reads==before+1);
        assert(last_information_kind==(panel_information_kind_t)k);
        assert(lv_obj_get_scroll_y(ui->status_body)==0);
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
        if(k==5) fixture.rgb.eof=129U;
        click(ui->information_refresh);assert(reads==before+2);
        if(k==5) {
            assert(strstr(lv_label_get_text(ui->status_text),"DMA EOF: 129")!=NULL);
            lv_font_glyph_dsc_t glyph;
            assert(lv_font_get_glyph_dsc(&homey_panel_font_22,&glyph,0x00E5U,0));
            assert(lv_font_get_glyph_dsc(&homey_panel_font_22,&glyph,0x00E4U,0));
            assert(lv_font_get_glyph_dsc(&homey_panel_font_22,&glyph,0x00F6U,0));
        }
        assert(lv_obj_get_height(lv_obj_get_child(ui->information_refresh,0))<=30);
        capture(40U+k);
        if(k==4 || k==5){
            assert(lv_obj_get_scroll_bottom(ui->status_body)>0);
            lv_area_t body_area,footer_area,refresh_area;
            lv_obj_get_coords(ui->status_body,&body_area);
            lv_obj_get_coords(ui->status_close_footer,&footer_area);
            lv_obj_get_coords(ui->information_refresh,&refresh_area);
            assert(body_area.y2<footer_area.y1&&body_area.y2<refresh_area.y1);
            assert(!areas_overlap(ui->status_body,ui->status_close_footer));
            assert(!areas_overlap(ui->status_body,ui->information_refresh));
            lv_obj_scroll_to_y(ui->status_body,1000,LV_ANIM_OFF);draw();
            assert(lv_obj_get_scroll_y(ui->status_body)>0);
            capture(50U+k);
            click(ui->information_refresh);
            assert(lv_obj_get_scroll_y(ui->status_body)==0);
            assert(!areas_overlap(ui->status_body,ui->status_close_footer));
        }
        click(k%2?ui->status_close_footer:ui->status_close);assert(ui->modal_kind==MODAL_NONE);
    }
    click(button(ui->settings_advanced_body,labels[0]));fixture.wifi_online=false;
    click(ui->information_refresh);assert(strstr(lv_label_get_text(ui->status_text),"Frånkopplad"));
    fixture.wifi_online=true;click(ui->information_refresh);assert(strstr(lv_label_get_text(ui->status_text),"Ansluten"));
    present=false;click(ui->information_refresh);assert(strcmp(lv_label_get_text(ui->status_text),"Ej tillgänglig")==0);
    click(ui->status_close);present=true;
    /* Wi-Fi QR owns its existing object throughout info/error/open/close. */
    lv_obj_t *frame=lv_obj_create(ui->screen);
    lv_obj_t *qr=lv_qrcode_create(frame);
    assert(panel_wifi_qr_layout_frame(frame,qr,PANEL_WIFI_QR_STARTUP_SIZE));
    assert(panel_ui_update_wifi_provisioning(ui,qr,frame,"fixture-setup","synthetic-code",true));
    assert(panel_ui_present_wifi_provisioning(ui,qr,false));draw();
    assert(ui->modal_kind==MODAL_WIFI&&lv_obj_has_flag(ui->information_refresh,LV_OBJ_FLAG_HIDDEN));
    assert_color(ui->status_dialog,0xFFFFFF);
    assert(lv_color_eq(lv_obj_get_style_text_color(ui->wifi_ssid,0),lv_color_hex(0x111820)));
    click(ui->status_close);click(button(ui->settings_advanced_body,labels[4]));
    assert(ui->modal_kind==MODAL_INFORMATION&&lv_obj_has_flag(ui->wifi_qr,LV_OBJ_FLAG_HIDDEN));
    click(ui->status_close);assert(panel_ui_present_wifi_provisioning(ui,qr,false));click(ui->status_close);
    assert(panel_ui_clear_wifi_provisioning(ui,qr));
    lv_obj_delete(frame);
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
        roller_feedback_reject(ui,1U,(n&1U)!=0U);
        roller_feedback_reject(ui,2U,(n&1U)==0U);
        roller_feedback_cancel(ui);
    }
    lv_mem_monitor(&feedback_after);assert(feedback_after.free_size==feedback_before.free_size);
    assert(mutations==0);
    assert_theme_labels_valid(ui);
    printf("PATCH087_REAL_LVGL_SIX_VIEWS_REFRESH_QR_LIFECYCLE PASS frames=%u cycles=100 pool_delta=%ld\n",frames,(long)after.free_size-(long)before.free_size);
    roller_feedback_reject(ui,1U,false);
    panel_ui_destroy(&ui);assert(!ui);lv_display_delete(d);lv_deinit();
}
