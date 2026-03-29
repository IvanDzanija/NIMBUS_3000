#include "../ui.h"
#include "ui_message_bridge.h"

lv_obj_t *ui_PrimljenePoruke = NULL;
lv_obj_t *ui_PrimljenePorukeBack = NULL;
lv_obj_t *ui_PrimljenePorukePanel = NULL;
static lv_timer_t *ui_PrimljenePorukeRefreshTimer = NULL;
static unsigned int ui_PrimljenePorukeLastVersion = 0;

static bool ui_PrimljenePoruke_is_dark(void)
{
    return ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
}

static void ui_event_PrimljenePorukeBack(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Whats_Down, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Whats_Down_screen_init);
    }
}

static void ui_inbox_add_message_row(lv_obj_t *parent, const char *sender, const char *text)
{
    bool is_dark = ui_PrimljenePoruke_is_dark();
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(94));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_left(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(card, is_dark ? lv_color_hex(0x2A2A2A) : lv_color_hex(0xF2E0D2), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sender_label = lv_label_create(card);
    lv_label_set_text_fmt(sender_label, "Od: %s", sender);
    lv_obj_set_width(sender_label, lv_pct(100));
    lv_obj_set_style_text_font(sender_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(sender_label, is_dark ? lv_color_hex(0xB8C0CC) : lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *message_label = lv_label_create(card);
    lv_label_set_text(message_label, text);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(message_label, lv_pct(100));
    lv_obj_set_style_text_font(message_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(message_label, is_dark ? lv_color_hex(0xF5F5F5) : lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(message_label, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_PrimljenePoruke_timer_cb(lv_timer_t *timer)
{
    unsigned int version = ui_get_received_message_version();
    (void)timer;

    if (version != ui_PrimljenePorukeLastVersion) {
        ui_PrimljenePoruke_refresh();
    }
}

void ui_PrimljenePoruke_refresh(void)
{
    bool is_dark = ui_PrimljenePoruke_is_dark();
    int count;
    int index;

    if (ui_PrimljenePorukePanel == NULL) {
        return;
    }

    if (ui_PrimljenePoruke) {
        lv_obj_set_style_bg_color(ui_PrimljenePoruke, is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_set_style_bg_color(ui_PrimljenePorukePanel, is_dark ? lv_color_hex(0x1A1A1A) : lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clean(ui_PrimljenePorukePanel);

    count = ui_get_received_message_count();
    if (count == 0) {
        lv_obj_t *empty_label = lv_label_create(ui_PrimljenePorukePanel);
        lv_label_set_text(empty_label, "Jos nema primljenih poruka.");
        lv_obj_set_width(empty_label, lv_pct(100));
        lv_label_set_long_mode(empty_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(empty_label, is_dark ? lv_color_hex(0x9A9A9A) : lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_PrimljenePorukeLastVersion = ui_get_received_message_version();
        return;
    }

    for (index = count - 1; index >= 0; index--) {
        ui_inbox_add_message_row(
            ui_PrimljenePorukePanel,
            ui_get_received_message_sender(index),
            ui_get_received_message_text(index));
    }

    ui_PrimljenePorukeLastVersion = ui_get_received_message_version();
}

void ui_PrimljenePoruke_screen_init(void)
{
    bool is_dark = ui_PrimljenePoruke_is_dark();
    ui_PrimljenePoruke = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PrimljenePoruke, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PrimljenePoruke, is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PrimljenePoruke, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PrimljenePorukeBack = lv_btn_create(ui_PrimljenePoruke);
    lv_obj_set_size(ui_PrimljenePorukeBack, 44, 34);
    lv_obj_set_x(ui_PrimljenePorukeBack, -123);
    lv_obj_set_y(ui_PrimljenePorukeBack, -92);
    lv_obj_set_align(ui_PrimljenePorukeBack, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_PrimljenePorukeBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PrimljenePorukeBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_PrimljenePorukeBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_img = lv_img_create(ui_PrimljenePorukeBack);
    lv_img_set_src(back_img, &ui_img_back_button_png);
    lv_obj_center(back_img);
    lv_img_set_zoom(back_img, 110);

    lv_obj_t *title = lv_label_create(ui_PrimljenePoruke);
    lv_label_set_text(title, "Primljene poruke");
    lv_obj_set_x(title, 0);
    lv_obj_set_y(title, -92);
    lv_obj_set_align(title, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, is_dark ? lv_color_hex(0xF5F5F5) : lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PrimljenePorukePanel = lv_obj_create(ui_PrimljenePoruke);
    lv_obj_set_size(ui_PrimljenePorukePanel, 236, 190);
    lv_obj_set_x(ui_PrimljenePorukePanel, 2);
    lv_obj_set_y(ui_PrimljenePorukePanel, 40);
    lv_obj_set_align(ui_PrimljenePorukePanel, LV_ALIGN_CENTER);
    lv_obj_set_layout(ui_PrimljenePorukePanel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ui_PrimljenePorukePanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_PrimljenePorukePanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(ui_PrimljenePorukePanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_PrimljenePorukePanel, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_pad_all(ui_PrimljenePorukePanel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui_PrimljenePorukePanel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PrimljenePorukePanel, is_dark ? lv_color_hex(0x1A1A1A) : lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PrimljenePorukePanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_PrimljenePorukePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_PrimljenePorukePanel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PrimljenePoruke_refresh();
    ui_PrimljenePorukeRefreshTimer = lv_timer_create(ui_PrimljenePoruke_timer_cb, 500, NULL);
    lv_obj_add_event_cb(ui_PrimljenePorukeBack, ui_event_PrimljenePorukeBack, LV_EVENT_ALL, NULL);
}

void ui_PrimljenePoruke_screen_destroy(void)
{
    if (ui_PrimljenePorukeRefreshTimer) {
        lv_timer_del(ui_PrimljenePorukeRefreshTimer);
        ui_PrimljenePorukeRefreshTimer = NULL;
    }

    if (ui_PrimljenePoruke) lv_obj_del(ui_PrimljenePoruke);

    ui_PrimljenePoruke = NULL;
    ui_PrimljenePorukeBack = NULL;
    ui_PrimljenePorukePanel = NULL;
    ui_PrimljenePorukeLastVersion = 0;
}
