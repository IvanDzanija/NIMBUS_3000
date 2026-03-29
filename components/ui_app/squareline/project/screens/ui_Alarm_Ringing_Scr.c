#include "../ui.h"
#include "ui_message_bridge.h"

lv_obj_t *ui_Alarm_Ringing_Scr = NULL;
lv_obj_t *ui_Alarm_Ringing_Title = NULL;
lv_obj_t *ui_Alarm_Ringing_Subtitle = NULL;
lv_obj_t *ui_Alarm_StopBtn = NULL;
lv_obj_t *ui_Alarm_StopLabel = NULL;

static void ui_event_Alarm_Stop(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    ui_set_alarm_ringing(0);
    _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                      &ui_Home_Scr_screen_init);
}

void ui_Alarm_Ringing_Scr_refresh(void)
{
    bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    lv_color_t title_color = lv_color_hex(0xD92D20);
    lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);
    lv_color_t accent = lv_color_hex(0xEBA678);

    if (!ui_Alarm_Ringing_Scr) return;

    lv_obj_set_style_bg_color(ui_Alarm_Ringing_Scr, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Alarm_Ringing_Scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_Ringing_Title) lv_obj_set_style_text_color(ui_Alarm_Ringing_Title, title_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_Ringing_Subtitle) lv_obj_set_style_text_color(ui_Alarm_Ringing_Subtitle, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_StopBtn) {
        lv_obj_set_style_bg_color(ui_Alarm_StopBtn, accent, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Alarm_StopBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_Alarm_StopBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_Alarm_StopLabel) lv_obj_set_style_text_color(ui_Alarm_StopLabel, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Alarm_Ringing_Scr_screen_init(void)
{
    ui_Alarm_Ringing_Scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Alarm_Ringing_Scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_Alarm_Ringing_Title = lv_label_create(ui_Alarm_Ringing_Scr);
    lv_obj_set_width(ui_Alarm_Ringing_Title, 220);
    lv_obj_set_align(ui_Alarm_Ringing_Title, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_Ringing_Title, -46);
    lv_obj_set_style_text_align(ui_Alarm_Ringing_Title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Alarm_Ringing_Title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Alarm_Ringing_Title, "Alarm zvoni!");

    ui_Alarm_Ringing_Subtitle = lv_label_create(ui_Alarm_Ringing_Scr);
    lv_obj_set_width(ui_Alarm_Ringing_Subtitle, 240);
    lv_obj_set_align(ui_Alarm_Ringing_Subtitle, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_Ringing_Subtitle, 10);
    lv_obj_set_style_text_align(ui_Alarm_Ringing_Subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Alarm_Ringing_Subtitle, "Stisni tipku ispod da ugasis alarm");

    ui_Alarm_StopBtn = lv_btn_create(ui_Alarm_Ringing_Scr);
    lv_obj_set_size(ui_Alarm_StopBtn, 150, 48);
    lv_obj_set_align(ui_Alarm_StopBtn, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_StopBtn, 82);

    ui_Alarm_StopLabel = lv_label_create(ui_Alarm_StopBtn);
    lv_obj_set_align(ui_Alarm_StopLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Alarm_StopLabel, "Ugasi alarm");

    lv_obj_add_event_cb(ui_Alarm_StopBtn, ui_event_Alarm_Stop, LV_EVENT_ALL, NULL);
    ui_Alarm_Ringing_Scr_refresh();
}

void ui_Alarm_Ringing_Scr_screen_destroy(void)
{
    if (ui_Alarm_Ringing_Scr) lv_obj_del(ui_Alarm_Ringing_Scr);

    ui_Alarm_Ringing_Scr = NULL;
    ui_Alarm_Ringing_Title = NULL;
    ui_Alarm_Ringing_Subtitle = NULL;
    ui_Alarm_StopBtn = NULL;
    ui_Alarm_StopLabel = NULL;
}
