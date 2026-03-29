#include "../ui.h"
#include "ui_message_bridge.h"

lv_obj_t *ui_Alarm_Scr = NULL;
lv_obj_t *ui_Alarm_Back = NULL;
lv_obj_t *ui_Alarm_BackImg = NULL;
lv_obj_t *ui_Alarm_Title = NULL;
lv_obj_t *ui_Alarm_HourRoller = NULL;
lv_obj_t *ui_Alarm_MinuteRoller = NULL;
lv_obj_t *ui_Alarm_Colon = NULL;
lv_obj_t *ui_Alarm_EnableSwitch = NULL;
lv_obj_t *ui_Alarm_EnableLabel = NULL;
lv_obj_t *ui_Alarm_SaveBtn = NULL;
lv_obj_t *ui_Alarm_SaveLabel = NULL;
lv_obj_t *ui_Alarm_Status = NULL;

static void ui_Alarm_update_status(void)
{
    if (!ui_Alarm_Status) return;

    if (ui_is_alarm_enabled()) {
        lv_label_set_text_fmt(ui_Alarm_Status, "Alarm je postavljen na %02d:%02d",
                              ui_get_alarm_hour(), ui_get_alarm_minute());
    } else {
        lv_label_set_text(ui_Alarm_Status, "Alarm je iskljucen");
    }
}

static void ui_event_Alarm_Back(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                      &ui_Home_Scr_screen_init);
}

static void ui_event_Alarm_Save(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    int hour = lv_roller_get_selected(ui_Alarm_HourRoller);
    int minute = lv_roller_get_selected(ui_Alarm_MinuteRoller);
    int enabled = lv_obj_has_state(ui_Alarm_EnableSwitch, LV_STATE_CHECKED);

    ui_set_alarm_time(hour, minute);
    ui_set_alarm_enabled(enabled);
    ui_Alarm_update_status();
}

void ui_Alarm_Scr_refresh(void)
{
    bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);
    lv_color_t panel_bg = is_dark ? lv_color_hex(0x1E1E1E) : lv_color_hex(0xFFFFFF);
    lv_color_t accent = lv_color_hex(0xEBA678);

    if (!ui_Alarm_Scr) return;

    lv_obj_set_style_bg_color(ui_Alarm_Scr, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Alarm_Scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (ui_Alarm_Title) lv_obj_set_style_text_color(ui_Alarm_Title, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_Colon) lv_obj_set_style_text_color(ui_Alarm_Colon, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_EnableLabel) lv_obj_set_style_text_color(ui_Alarm_EnableLabel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_Alarm_Status) lv_obj_set_style_text_color(ui_Alarm_Status, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (ui_Alarm_HourRoller) {
        lv_obj_set_style_bg_color(ui_Alarm_HourRoller, panel_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_Alarm_HourRoller, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Alarm_HourRoller, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Alarm_HourRoller, accent, LV_PART_SELECTED | LV_STATE_DEFAULT);
    }
    if (ui_Alarm_MinuteRoller) {
        lv_obj_set_style_bg_color(ui_Alarm_MinuteRoller, panel_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_Alarm_MinuteRoller, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Alarm_MinuteRoller, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Alarm_MinuteRoller, accent, LV_PART_SELECTED | LV_STATE_DEFAULT);
    }
    if (ui_Alarm_SaveBtn) {
        lv_obj_set_style_bg_color(ui_Alarm_SaveBtn, accent, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Alarm_SaveBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_Alarm_SaveBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_Alarm_SaveLabel) lv_obj_set_style_text_color(ui_Alarm_SaveLabel, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Alarm_Scr_screen_init(void)
{
    static const char *hour_options =
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";
    static const char *minute_options =
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59";

    ui_Alarm_Scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Alarm_Scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_Alarm_Back = lv_btn_create(ui_Alarm_Scr);
    lv_obj_set_width(ui_Alarm_Back, 40);
    lv_obj_set_height(ui_Alarm_Back, 30);
    lv_obj_set_x(ui_Alarm_Back, -124);
    lv_obj_set_y(ui_Alarm_Back, -96);
    lv_obj_set_align(ui_Alarm_Back, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_Alarm_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Alarm_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_Alarm_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_Alarm_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Alarm_BackImg = lv_img_create(ui_Alarm_Back);
    lv_img_set_src(ui_Alarm_BackImg, &ui_img_back_button_png);
    lv_obj_set_align(ui_Alarm_BackImg, LV_ALIGN_CENTER);
    lv_img_set_zoom(ui_Alarm_BackImg, 110);

    ui_Alarm_Title = lv_label_create(ui_Alarm_Scr);
    lv_obj_set_align(ui_Alarm_Title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_Alarm_Title, 18);
    lv_label_set_text(ui_Alarm_Title, "Alarm");
    lv_obj_set_style_text_font(ui_Alarm_Title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Alarm_HourRoller = lv_roller_create(ui_Alarm_Scr);
    lv_roller_set_options(ui_Alarm_HourRoller, hour_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(ui_Alarm_HourRoller, 70, 110);
    lv_obj_set_x(ui_Alarm_HourRoller, -46);
    lv_obj_set_y(ui_Alarm_HourRoller, -12);
    lv_obj_set_align(ui_Alarm_HourRoller, LV_ALIGN_CENTER);
    lv_roller_set_visible_row_count(ui_Alarm_HourRoller, 3);

    ui_Alarm_Colon = lv_label_create(ui_Alarm_Scr);
    lv_obj_set_align(ui_Alarm_Colon, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_Colon, -12);
    lv_label_set_text(ui_Alarm_Colon, ":");
    lv_obj_set_style_text_font(ui_Alarm_Colon, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Alarm_MinuteRoller = lv_roller_create(ui_Alarm_Scr);
    lv_roller_set_options(ui_Alarm_MinuteRoller, minute_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(ui_Alarm_MinuteRoller, 70, 110);
    lv_obj_set_x(ui_Alarm_MinuteRoller, 46);
    lv_obj_set_y(ui_Alarm_MinuteRoller, -12);
    lv_obj_set_align(ui_Alarm_MinuteRoller, LV_ALIGN_CENTER);
    lv_roller_set_visible_row_count(ui_Alarm_MinuteRoller, 3);

    ui_Alarm_EnableLabel = lv_label_create(ui_Alarm_Scr);
    lv_obj_set_align(ui_Alarm_EnableLabel, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Alarm_EnableLabel, -34);
    lv_obj_set_y(ui_Alarm_EnableLabel, 72);
    lv_label_set_text(ui_Alarm_EnableLabel, "Ukljucen");

    ui_Alarm_EnableSwitch = lv_switch_create(ui_Alarm_Scr);
    lv_obj_set_align(ui_Alarm_EnableSwitch, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Alarm_EnableSwitch, 46);
    lv_obj_set_y(ui_Alarm_EnableSwitch, 72);

    ui_Alarm_SaveBtn = lv_btn_create(ui_Alarm_Scr);
    lv_obj_set_size(ui_Alarm_SaveBtn, 120, 40);
    lv_obj_set_align(ui_Alarm_SaveBtn, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_SaveBtn, 122);

    ui_Alarm_SaveLabel = lv_label_create(ui_Alarm_SaveBtn);
    lv_obj_set_align(ui_Alarm_SaveLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Alarm_SaveLabel, "Spremi");

    ui_Alarm_Status = lv_label_create(ui_Alarm_Scr);
    lv_obj_set_width(ui_Alarm_Status, 220);
    lv_obj_set_align(ui_Alarm_Status, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Alarm_Status, 156);
    lv_obj_set_style_text_align(ui_Alarm_Status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_roller_set_selected(ui_Alarm_HourRoller, ui_get_alarm_hour(), LV_ANIM_OFF);
    lv_roller_set_selected(ui_Alarm_MinuteRoller, ui_get_alarm_minute(), LV_ANIM_OFF);
    if (ui_is_alarm_enabled()) {
        lv_obj_add_state(ui_Alarm_EnableSwitch, LV_STATE_CHECKED);
    }
    ui_Alarm_update_status();

    lv_obj_add_event_cb(ui_Alarm_Back, ui_event_Alarm_Back, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Alarm_SaveBtn, ui_event_Alarm_Save, LV_EVENT_ALL, NULL);
    ui_Alarm_Scr_refresh();
}

void ui_Alarm_Scr_screen_destroy(void)
{
    if (ui_Alarm_Scr) lv_obj_del(ui_Alarm_Scr);

    ui_Alarm_Scr = NULL;
    ui_Alarm_Back = NULL;
    ui_Alarm_BackImg = NULL;
    ui_Alarm_Title = NULL;
    ui_Alarm_HourRoller = NULL;
    ui_Alarm_MinuteRoller = NULL;
    ui_Alarm_Colon = NULL;
    ui_Alarm_EnableSwitch = NULL;
    ui_Alarm_EnableLabel = NULL;
    ui_Alarm_SaveBtn = NULL;
    ui_Alarm_SaveLabel = NULL;
    ui_Alarm_Status = NULL;
}
