#include "../ui.h"

lv_obj_t *ui_KriviOdgovor = NULL;
lv_obj_t *ui_KriviBack = NULL;
lv_obj_t *ui_KriviBackImg = NULL;
lv_obj_t *ui_KriviTitle = NULL;
lv_obj_t *ui_KriviSubtitle = NULL;

static void ui_event_KriviBack(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                      &ui_Home_Scr_screen_init);
}

void ui_KriviOdgovor_refresh(void)
{
    bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    lv_color_t title_color = is_dark ? lv_color_hex(0xFF9A9A) : lv_color_hex(0xB42318);
    lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);

    if (!ui_KriviOdgovor) {
        return;
    }

    lv_obj_set_style_bg_color(ui_KriviOdgovor, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_KriviOdgovor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (ui_KriviTitle) {
        lv_obj_set_style_text_color(ui_KriviTitle, title_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_KriviSubtitle) {
        lv_obj_set_style_text_color(ui_KriviSubtitle, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_KriviOdgovor_screen_init(void)
{
    ui_KriviOdgovor = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_KriviOdgovor, LV_OBJ_FLAG_SCROLLABLE);

    ui_KriviBack = lv_btn_create(ui_KriviOdgovor);
    lv_obj_set_width(ui_KriviBack, 40);
    lv_obj_set_height(ui_KriviBack, 30);
    lv_obj_set_x(ui_KriviBack, -124);
    lv_obj_set_y(ui_KriviBack, -96);
    lv_obj_set_align(ui_KriviBack, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_KriviBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_KriviBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_KriviBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_KriviBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_KriviBackImg = lv_img_create(ui_KriviBack);
    lv_img_set_src(ui_KriviBackImg, &ui_img_back_button_png);
    lv_obj_set_align(ui_KriviBackImg, LV_ALIGN_CENTER);
    lv_img_set_zoom(ui_KriviBackImg, 110);

    ui_KriviTitle = lv_label_create(ui_KriviOdgovor);
    lv_obj_set_width(ui_KriviTitle, 220);
    lv_obj_set_x(ui_KriviTitle, 0);
    lv_obj_set_y(ui_KriviTitle, -20);
    lv_obj_set_align(ui_KriviTitle, LV_ALIGN_CENTER);
    lv_label_set_text(ui_KriviTitle, "Krivi odgovor!");
    lv_label_set_long_mode(ui_KriviTitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui_KriviTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_KriviTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_KriviSubtitle = lv_label_create(ui_KriviOdgovor);
    lv_obj_set_width(ui_KriviSubtitle, 240);
    lv_obj_set_x(ui_KriviSubtitle, 0);
    lv_obj_set_y(ui_KriviSubtitle, 36);
    lv_obj_set_align(ui_KriviSubtitle, LV_ALIGN_CENTER);
    lv_label_set_long_mode(ui_KriviSubtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui_KriviSubtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_fmt(ui_KriviSubtitle, "Tocno je bilo: %s", animal_game_get_current_name());

    lv_obj_add_event_cb(ui_KriviBack, ui_event_KriviBack, LV_EVENT_ALL, NULL);
    ui_KriviOdgovor_refresh();
}

void ui_KriviOdgovor_screen_destroy(void)
{
    if (ui_KriviOdgovor) lv_obj_del(ui_KriviOdgovor);

    ui_KriviOdgovor = NULL;
    ui_KriviBack = NULL;
    ui_KriviBackImg = NULL;
    ui_KriviTitle = NULL;
    ui_KriviSubtitle = NULL;
}
