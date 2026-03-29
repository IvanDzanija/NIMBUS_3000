#include "../ui.h"

lv_obj_t *ui_TocanOdgovor = NULL;
lv_obj_t *ui_TocanBack = NULL;
lv_obj_t *ui_TocanBackImg = NULL;
lv_obj_t *ui_TocanTitle = NULL;
lv_obj_t *ui_TocanSubtitle = NULL;

static void ui_event_TocanBack(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                      &ui_Home_Scr_screen_init);
}

void ui_TocanOdgovor_refresh(void)
{
    bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    lv_color_t title_color = is_dark ? lv_color_hex(0x9EF0A8) : lv_color_hex(0x1F7A35);
    lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);

    if (!ui_TocanOdgovor) {
        return;
    }

    lv_obj_set_style_bg_color(ui_TocanOdgovor, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TocanOdgovor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (ui_TocanTitle) {
        lv_obj_set_style_text_color(ui_TocanTitle, title_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_TocanSubtitle) {
        lv_obj_set_style_text_color(ui_TocanSubtitle, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_TocanOdgovor_screen_init(void)
{
    ui_TocanOdgovor = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_TocanOdgovor, LV_OBJ_FLAG_SCROLLABLE);

    ui_TocanBack = lv_btn_create(ui_TocanOdgovor);
    lv_obj_set_width(ui_TocanBack, 40);
    lv_obj_set_height(ui_TocanBack, 30);
    lv_obj_set_x(ui_TocanBack, -124);
    lv_obj_set_y(ui_TocanBack, -96);
    lv_obj_set_align(ui_TocanBack, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_TocanBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_TocanBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_TocanBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_TocanBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TocanBackImg = lv_img_create(ui_TocanBack);
    lv_img_set_src(ui_TocanBackImg, &ui_img_back_button_png);
    lv_obj_set_align(ui_TocanBackImg, LV_ALIGN_CENTER);
    lv_img_set_zoom(ui_TocanBackImg, 110);

    ui_TocanTitle = lv_label_create(ui_TocanOdgovor);
    lv_obj_set_width(ui_TocanTitle, 220);
    lv_obj_set_x(ui_TocanTitle, 0);
    lv_obj_set_y(ui_TocanTitle, -20);
    lv_obj_set_align(ui_TocanTitle, LV_ALIGN_CENTER);
    lv_label_set_text(ui_TocanTitle, "Tocan odgovor!");
    lv_label_set_long_mode(ui_TocanTitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui_TocanTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TocanTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TocanSubtitle = lv_label_create(ui_TocanOdgovor);
    lv_obj_set_width(ui_TocanSubtitle, 240);
    lv_obj_set_x(ui_TocanSubtitle, 0);
    lv_obj_set_y(ui_TocanSubtitle, 36);
    lv_obj_set_align(ui_TocanSubtitle, LV_ALIGN_CENTER);
    lv_label_set_long_mode(ui_TocanSubtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui_TocanSubtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_fmt(ui_TocanSubtitle, "To je bio: %s", animal_game_get_current_name());

    lv_obj_add_event_cb(ui_TocanBack, ui_event_TocanBack, LV_EVENT_ALL, NULL);
    ui_TocanOdgovor_refresh();
}

void ui_TocanOdgovor_screen_destroy(void)
{
    if (ui_TocanOdgovor) lv_obj_del(ui_TocanOdgovor);

    ui_TocanOdgovor = NULL;
    ui_TocanBack = NULL;
    ui_TocanBackImg = NULL;
    ui_TocanTitle = NULL;
    ui_TocanSubtitle = NULL;
}
