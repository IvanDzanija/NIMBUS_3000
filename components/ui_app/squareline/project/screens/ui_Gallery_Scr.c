#include "../ui.h"
#include "ui_camera_gallery.h"

#define GALLERY_ZOOM 6 * 256 / 2

lv_obj_t *ui_Gallery_Scr = NULL;

static lv_obj_t *s_back_btn = NULL;
static lv_obj_t *s_back_img = NULL;
static lv_obj_t *s_title = NULL;
static lv_obj_t *s_empty_label = NULL;
static lv_obj_t *s_thumb_btns[5];
static lv_obj_t *s_thumb_canvas[5];

static lv_obj_t *s_overlay = NULL;
static lv_obj_t *s_overlay_canvas = NULL;
static lv_obj_t *s_overlay_close = NULL;
static lv_obj_t *s_overlay_close_label = NULL;

static const lv_point_t s_thumb_pos[5] = {
    {-70, -36}, {0, -36}, {70, -36}, {-35, 44}, {35, 44},
};

void ui_Gallery_Scr_refresh(void)
{
    const bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    const lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    const lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);
    const lv_color_t hint_color = is_dark ? lv_color_hex(0x888888) : lv_color_hex(0x666666);
    const lv_color_t thumb_bg = is_dark ? lv_color_hex(0x2A2A2A) : lv_color_hex(0xFFFFFF);
    const lv_color_t thumb_border = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xD8B08C);
    const lv_color_t overlay_btn_bg = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xEBA678);

    if (ui_Gallery_Scr) {
        lv_obj_set_style_bg_color(ui_Gallery_Scr, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_title) {
        lv_obj_set_style_text_color(s_title, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_empty_label) {
        lv_obj_set_style_text_color(s_empty_label, hint_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    for (uint8_t i = 0; i < 5; ++i) {
        if (s_thumb_btns[i]) {
            lv_obj_set_style_bg_color(s_thumb_btns[i], thumb_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(s_thumb_btns[i], thumb_border, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    if (s_overlay_close) {
        lv_obj_set_style_bg_color(s_overlay_close, overlay_btn_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_overlay_close, is_dark ? 0 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(s_overlay_close, lv_color_hex(0xD08C5B),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_overlay_close_label) {
        lv_obj_set_style_text_color(s_overlay_close_label, lv_color_hex(0x111111),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void s_event_back(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                          &ui_Home_Scr_screen_init);
    }
}

static void s_event_close_overlay(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && s_overlay) {
        lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

static void s_event_open_image(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !s_overlay || !s_overlay_canvas) {
        return;
    }

    const uint8_t index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    const lv_color_t *buf = NULL;
    uint16_t w = 0;
    uint16_t h = 0;

    if (!ui_gallery_get_frame(index, &buf, &w, &h)) {
        return;
    }

    lv_canvas_set_buffer(s_overlay_canvas, (void *)buf, w, h, LV_IMG_CF_TRUE_COLOR);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
    lv_obj_invalidate(s_overlay_canvas);
}

static void s_refresh_gallery(void)
{
    const uint8_t count = ui_gallery_get_count();

    if (s_empty_label) {
        if (count == 0) {
            lv_obj_clear_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    for (uint8_t i = 0; i < 5; ++i) {
        if (!s_thumb_btns[i] || !s_thumb_canvas[i]) {
            continue;
        }

        if (i < count) {
            const lv_color_t *buf = NULL;
            uint16_t w = 0;
            uint16_t h = 0;
            ui_gallery_get_frame(i, &buf, &w, &h);
            lv_canvas_set_buffer(s_thumb_canvas[i], (void *)buf, w, h, LV_IMG_CF_TRUE_COLOR);
            lv_obj_clear_flag(s_thumb_btns[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(s_thumb_canvas[i]);
        } else {
            lv_obj_add_flag(s_thumb_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_Gallery_Scr_screen_init(void)
{
    ui_Gallery_Scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Gallery_Scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Gallery_Scr, lv_color_hex(0x111111),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Gallery_Scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_back_btn = lv_btn_create(ui_Gallery_Scr);
    lv_obj_set_size(s_back_btn, 40, 30);
    lv_obj_set_x(s_back_btn, -124);
    lv_obj_set_y(s_back_btn, -96);
    lv_obj_set_align(s_back_btn, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_back_btn, s_event_back, LV_EVENT_ALL, NULL);

    s_back_img = lv_img_create(s_back_btn);
    lv_img_set_src(s_back_img, &ui_img_back_button_png);
    lv_obj_set_align(s_back_img, LV_ALIGN_CENTER);
    lv_img_set_zoom(s_back_img, 110);

    s_title = lv_label_create(ui_Gallery_Scr);
    lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_title, 18);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_title, lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_title, "Galerija");

    s_empty_label = lv_label_create(ui_Gallery_Scr);
    lv_obj_set_align(s_empty_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(s_empty_label, lv_color_hex(0x888888),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_empty_label, "Jos nema spremljenih slika");

    for (uint8_t i = 0; i < 5; ++i) {
        s_thumb_btns[i] = lv_btn_create(ui_Gallery_Scr);
        lv_obj_set_size(s_thumb_btns[i], 56, 46);
        lv_obj_set_align(s_thumb_btns[i], LV_ALIGN_CENTER);
        lv_obj_set_x(s_thumb_btns[i], s_thumb_pos[i].x);
        lv_obj_set_y(s_thumb_btns[i], s_thumb_pos[i].y);
        lv_obj_set_style_bg_color(s_thumb_btns[i], lv_color_hex(0x2A2A2A),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(s_thumb_btns[i], lv_color_hex(0xFFFFFF),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_thumb_btns[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(s_thumb_btns[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(s_thumb_btns[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(s_thumb_btns[i], s_event_open_image, LV_EVENT_ALL,
                            (void *)(uintptr_t)i);

        s_thumb_canvas[i] = lv_canvas_create(s_thumb_btns[i]);
        lv_obj_center(s_thumb_canvas[i]);
    }

    s_overlay = lv_obj_create(ui_Gallery_Scr);
    lv_obj_set_size(s_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    s_overlay_canvas = lv_canvas_create(s_overlay);
    lv_obj_set_align(s_overlay_canvas, LV_ALIGN_CENTER);
    lv_img_set_zoom(s_overlay_canvas, 1536);

    s_overlay_close = lv_btn_create(s_overlay);
    lv_obj_set_size(s_overlay_close, 72, 38);
    lv_obj_set_align(s_overlay_close, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_overlay_close, -16);
    lv_obj_set_style_bg_color(s_overlay_close, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_overlay_close, lv_color_hex(0x111111),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(s_overlay_close, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_overlay_close, s_event_close_overlay, LV_EVENT_ALL, NULL);

    s_overlay_close_label = lv_label_create(s_overlay_close);
    lv_label_set_text(s_overlay_close_label, "Zatvori");
    lv_obj_center(s_overlay_close_label);

    s_refresh_gallery();
    ui_Gallery_Scr_refresh();
}

void ui_Gallery_Scr_screen_destroy(void)
{
    if (ui_Gallery_Scr) {
        lv_obj_del(ui_Gallery_Scr);
    }

    ui_Gallery_Scr = NULL;
    s_back_btn = NULL;
    s_back_img = NULL;
    s_title = NULL;
    s_empty_label = NULL;
    s_overlay = NULL;
    s_overlay_canvas = NULL;
    s_overlay_close = NULL;
    s_overlay_close_label = NULL;
    for (uint8_t i = 0; i < 5; ++i) {
        s_thumb_btns[i] = NULL;
        s_thumb_canvas[i] = NULL;
    }
}
