#include "../ui.h"
#include "../ui_helpers.h"
#include "../../../../../components/lvgl/src/draw/lv_draw.h"
#include "ui_message_bridge.h"

#include <stdio.h>

#define UI_SKETCHPAD_MAX_POINTS 512
#define UI_SKETCHPAD_BREAK_POINT -32768

lv_obj_t *ui_Sketchpad_Scr = NULL;
lv_obj_t *ui_Sketchpad_Back = NULL;
lv_obj_t *ui_Sketchpad_BackImg = NULL;
lv_obj_t *ui_Sketchpad_Title = NULL;
lv_obj_t *ui_Sketchpad_Canvas = NULL;
static lv_obj_t *ui_Sketchpad_Hint = NULL;
static lv_obj_t *ui_Sketchpad_ClearBtn = NULL;
static lv_obj_t *ui_Sketchpad_ClearLabel = NULL;
static lv_obj_t *ui_Sketchpad_ColorBtn = NULL;
static lv_obj_t *ui_Sketchpad_ColorLabel = NULL;
static lv_obj_t *ui_Sketchpad_ColorStatus = NULL;

static lv_point_t ui_Sketchpad_Points[UI_SKETCHPAD_MAX_POINTS];
static uint16_t ui_Sketchpad_PointCount = 0;
static bool ui_Sketchpad_IsDrawing = false;
static lv_color_t ui_Sketchpad_DrawColor;
static uint32_t ui_Sketchpad_DrawColorHex = 0x1D3557;
static char ui_Sketchpad_DrawColorName[24] = "PLAVA";

static bool ui_Sketchpad_is_break_point(lv_point_t point)
{
    return point.x == UI_SKETCHPAD_BREAK_POINT &&
           point.y == UI_SKETCHPAD_BREAK_POINT;
}

static void ui_Sketchpad_reset_points(void)
{
    ui_Sketchpad_PointCount = 0;
    ui_Sketchpad_IsDrawing = false;
    if (ui_Sketchpad_Canvas) {
        lv_obj_invalidate(ui_Sketchpad_Canvas);
    }
    if (ui_Sketchpad_Hint) {
        lv_obj_clear_flag(ui_Sketchpad_Hint, LV_OBJ_FLAG_HIDDEN);
    }

    if (ui_Sketchpad_ColorStatus) {
        lv_label_set_text_fmt(ui_Sketchpad_ColorStatus, "Boja crtanja: %s", ui_Sketchpad_DrawColorName);
    }
}

static void ui_Sketchpad_apply_draw_color(uint32_t color_hex, const char *color_name)
{
    ui_Sketchpad_DrawColorHex = color_hex;
    ui_Sketchpad_DrawColor = lv_color_hex(color_hex);

    if (color_name != NULL && color_name[0] != '\0') {
        snprintf(ui_Sketchpad_DrawColorName, sizeof(ui_Sketchpad_DrawColorName), "%s", color_name);
    }

    if (ui_Sketchpad_ColorBtn) {
        lv_obj_set_style_bg_color(ui_Sketchpad_ColorBtn, ui_Sketchpad_DrawColor,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ColorStatus) {
        lv_label_set_text_fmt(ui_Sketchpad_ColorStatus, "Boja crtanja: %s", ui_Sketchpad_DrawColorName);
        lv_obj_set_style_text_color(ui_Sketchpad_ColorStatus, ui_Sketchpad_DrawColor,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_Canvas) {
        lv_obj_invalidate(ui_Sketchpad_Canvas);
    }
}

static bool ui_Sketchpad_get_local_point(lv_obj_t *obj, lv_point_t *point_out)
{
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point;
    lv_area_t coords;

    if (obj == NULL || point_out == NULL || indev == NULL) {
        return false;
    }

    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(obj, &coords);
    point.x -= coords.x1;
    point.y -= coords.y1;

    if (point.x < 0 || point.y < 0 ||
        point.x >= lv_obj_get_width(obj) || point.y >= lv_obj_get_height(obj)) {
        return false;
    }

    *point_out = point;
    return true;
}

static void ui_Sketchpad_add_point(lv_point_t point)
{
    if (ui_Sketchpad_PointCount >= UI_SKETCHPAD_MAX_POINTS) {
        return;
    }

    ui_Sketchpad_Points[ui_Sketchpad_PointCount++] = point;

    if (ui_Sketchpad_Hint) {
        lv_obj_add_flag(ui_Sketchpad_Hint, LV_OBJ_FLAG_HIDDEN);
    }

    if (ui_Sketchpad_Canvas) {
        lv_obj_invalidate(ui_Sketchpad_Canvas);
    }
}

static void ui_Sketchpad_add_break_point(void)
{
    lv_point_t break_point = {
        .x = UI_SKETCHPAD_BREAK_POINT,
        .y = UI_SKETCHPAD_BREAK_POINT,
    };

    if (ui_Sketchpad_PointCount == 0) {
        return;
    }

    if (ui_Sketchpad_is_break_point(ui_Sketchpad_Points[ui_Sketchpad_PointCount - 1])) {
        return;
    }

    ui_Sketchpad_add_point(break_point);
}

static void ui_event_Sketchpad_Back(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                      &ui_Home_Scr_screen_init);
}

static void ui_event_Sketchpad_Clear(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_Sketchpad_reset_points();
}

static void ui_event_Sketchpad_Color(lv_event_t * e)
{
    uint32_t color_hex = 0;
    char color_name[24] = {0};

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    if (!ui_request_sketchpad_sensor_color(&color_hex, color_name, sizeof(color_name))) {
        if (ui_Sketchpad_ColorStatus) {
            lv_label_set_text(ui_Sketchpad_ColorStatus, "Senzor boje nije dostupan");
            lv_obj_set_style_text_color(ui_Sketchpad_ColorStatus, lv_color_hex(0xB00020),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        return;
    }

    ui_Sketchpad_apply_draw_color(color_hex, color_name);
}

static void ui_event_Sketchpad_Canvas(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *canvas = lv_event_get_target(e);
    lv_point_t point;

    if (event_code == LV_EVENT_DRAW_MAIN) {
        lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
        lv_draw_line_dsc_t line_dsc;
        lv_area_t area;
        lv_coord_t x_ofs;
        lv_coord_t y_ofs;
        uint16_t i;

        if (draw_ctx == NULL || ui_Sketchpad_PointCount < 2) {
            return;
        }

        lv_obj_get_coords(canvas, &area);
        x_ofs = area.x1 - lv_obj_get_scroll_x(canvas);
        y_ofs = area.y1 - lv_obj_get_scroll_y(canvas);

        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = ui_Sketchpad_DrawColor;
        line_dsc.width = 4;
        line_dsc.opa = LV_OPA_COVER;
        line_dsc.round_start = 1;
        line_dsc.round_end = 1;

        for (i = 0; i < (ui_Sketchpad_PointCount - 1); i++) {
            if (ui_Sketchpad_is_break_point(ui_Sketchpad_Points[i]) ||
                ui_Sketchpad_is_break_point(ui_Sketchpad_Points[i + 1])) {
                line_dsc.round_start = 1;
                continue;
            }

            lv_point_t p1 = {
                .x = ui_Sketchpad_Points[i].x + x_ofs,
                .y = ui_Sketchpad_Points[i].y + y_ofs,
            };
            lv_point_t p2 = {
                .x = ui_Sketchpad_Points[i + 1].x + x_ofs,
                .y = ui_Sketchpad_Points[i + 1].y + y_ofs,
            };
            lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
            line_dsc.round_start = 0;
        }
        return;
    }

    if (event_code == LV_EVENT_RELEASED || event_code == LV_EVENT_PRESS_LOST) {
        ui_Sketchpad_IsDrawing = false;
        ui_Sketchpad_add_break_point();
        return;
    }

    if (event_code != LV_EVENT_PRESSED && event_code != LV_EVENT_PRESSING) {
        return;
    }

    if (!ui_Sketchpad_get_local_point(canvas, &point)) {
        ui_Sketchpad_IsDrawing = false;
        return;
    }

    if (!ui_Sketchpad_IsDrawing && ui_Sketchpad_PointCount < UI_SKETCHPAD_MAX_POINTS) {
        if (ui_Sketchpad_PointCount > 0) {
            ui_Sketchpad_Points[ui_Sketchpad_PointCount++] = point;
        }
        ui_Sketchpad_IsDrawing = true;
    }

    ui_Sketchpad_add_point(point);
}

void ui_Sketchpad_Scr_refresh(void)
{
    bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);
    lv_color_t panel_bg = lv_color_hex(0xFFFFFF);
    lv_color_t panel_border = is_dark ? lv_color_hex(0x5A5A5A) : lv_color_hex(0xD9D9D9);
    lv_color_t accent = lv_color_hex(0xEBA678);

    if (!ui_Sketchpad_Scr) return;

    lv_obj_set_style_bg_color(ui_Sketchpad_Scr, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Sketchpad_Scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (ui_Sketchpad_Title) {
        lv_obj_set_style_text_color(ui_Sketchpad_Title, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_Canvas) {
        lv_obj_set_style_bg_color(ui_Sketchpad_Canvas, panel_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Sketchpad_Canvas, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_Sketchpad_Canvas, panel_border, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_Sketchpad_Canvas, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_Sketchpad_Canvas, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_Sketchpad_Canvas, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(ui_Sketchpad_Canvas, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_Hint) {
        lv_obj_set_style_text_color(ui_Sketchpad_Hint, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ClearBtn) {
        lv_obj_set_style_bg_color(ui_Sketchpad_ClearBtn, accent, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Sketchpad_ClearBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Sketchpad_ClearBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_Sketchpad_ClearBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ClearLabel) {
        lv_obj_set_style_text_color(ui_Sketchpad_ClearLabel, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ColorBtn) {
        lv_obj_set_style_bg_color(ui_Sketchpad_ColorBtn, ui_Sketchpad_DrawColor, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Sketchpad_ColorBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_Sketchpad_ColorBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_Sketchpad_ColorBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ColorLabel) {
        lv_obj_set_style_text_color(ui_Sketchpad_ColorLabel, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_Sketchpad_ColorStatus) {
        lv_obj_set_style_text_color(ui_Sketchpad_ColorStatus, ui_Sketchpad_DrawColor, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_Sketchpad_Scr_screen_init(void)
{
    ui_Sketchpad_Scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Sketchpad_Scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_Sketchpad_Back = lv_btn_create(ui_Sketchpad_Scr);
    lv_obj_set_width(ui_Sketchpad_Back, 40);
    lv_obj_set_height(ui_Sketchpad_Back, 30);
    lv_obj_set_x(ui_Sketchpad_Back, -124);
    lv_obj_set_y(ui_Sketchpad_Back, -96);
    lv_obj_set_align(ui_Sketchpad_Back, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_Sketchpad_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Sketchpad_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_Sketchpad_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_Sketchpad_Back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Sketchpad_BackImg = lv_img_create(ui_Sketchpad_Back);
    lv_img_set_src(ui_Sketchpad_BackImg, &ui_img_back_button_png);
    lv_obj_set_align(ui_Sketchpad_BackImg, LV_ALIGN_CENTER);
    lv_img_set_zoom(ui_Sketchpad_BackImg, 110);

    ui_Sketchpad_Title = lv_label_create(ui_Sketchpad_Scr);
    lv_obj_set_align(ui_Sketchpad_Title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_Sketchpad_Title, 18);
    lv_obj_set_style_text_font(ui_Sketchpad_Title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Sketchpad_Title, "Sketchpad");

    ui_Sketchpad_Canvas = lv_obj_create(ui_Sketchpad_Scr);
    lv_obj_set_size(ui_Sketchpad_Canvas, 190, 150);
    lv_obj_set_align(ui_Sketchpad_Canvas, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_Sketchpad_Canvas, 18);
    lv_obj_clear_flag(ui_Sketchpad_Canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_Sketchpad_Canvas, LV_OBJ_FLAG_CLICKABLE);

    ui_Sketchpad_Hint = lv_label_create(ui_Sketchpad_Canvas);
    lv_obj_set_width(ui_Sketchpad_Hint, 150);
    lv_obj_set_align(ui_Sketchpad_Hint, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(ui_Sketchpad_Hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Sketchpad_Hint, "Crtaj prstom po bijelom polju");

    ui_Sketchpad_ColorStatus = lv_label_create(ui_Sketchpad_Scr);
    lv_obj_set_width(ui_Sketchpad_ColorStatus, 220);
    lv_obj_set_align(ui_Sketchpad_ColorStatus, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_Sketchpad_ColorStatus, -60);
    lv_obj_set_style_text_align(ui_Sketchpad_ColorStatus, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Sketchpad_ColorStatus, "Boja crtanja: PLAVA");

    ui_Sketchpad_ColorBtn = lv_btn_create(ui_Sketchpad_Scr);
    lv_obj_set_size(ui_Sketchpad_ColorBtn, 86, 34);
    lv_obj_set_align(ui_Sketchpad_ColorBtn, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(ui_Sketchpad_ColorBtn, 18);
    lv_obj_set_y(ui_Sketchpad_ColorBtn, -18);

    ui_Sketchpad_ColorLabel = lv_label_create(ui_Sketchpad_ColorBtn);
    lv_obj_set_align(ui_Sketchpad_ColorLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Sketchpad_ColorLabel, "Boja");

    ui_Sketchpad_ClearBtn = lv_btn_create(ui_Sketchpad_Scr);
    lv_obj_set_size(ui_Sketchpad_ClearBtn, 86, 34);
    lv_obj_set_align(ui_Sketchpad_ClearBtn, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_Sketchpad_ClearBtn, -18);
    lv_obj_set_y(ui_Sketchpad_ClearBtn, -18);

    ui_Sketchpad_ClearLabel = lv_label_create(ui_Sketchpad_ClearBtn);
    lv_obj_set_align(ui_Sketchpad_ClearLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Sketchpad_ClearLabel, "Obrisi");

    lv_obj_add_event_cb(ui_Sketchpad_Back, ui_event_Sketchpad_Back, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Sketchpad_ColorBtn, ui_event_Sketchpad_Color, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Sketchpad_ClearBtn, ui_event_Sketchpad_Clear, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Sketchpad_Canvas, ui_event_Sketchpad_Canvas, LV_EVENT_ALL, NULL);

    ui_Sketchpad_apply_draw_color(ui_Sketchpad_DrawColorHex, ui_Sketchpad_DrawColorName);
    ui_Sketchpad_reset_points();
    ui_Sketchpad_Scr_refresh();
}

void ui_Sketchpad_Scr_screen_destroy(void)
{
    if (ui_Sketchpad_Scr) lv_obj_del(ui_Sketchpad_Scr);

    ui_Sketchpad_Scr = NULL;
    ui_Sketchpad_Back = NULL;
    ui_Sketchpad_BackImg = NULL;
    ui_Sketchpad_Title = NULL;
    ui_Sketchpad_Canvas = NULL;
    ui_Sketchpad_Hint = NULL;
    ui_Sketchpad_ColorBtn = NULL;
    ui_Sketchpad_ColorLabel = NULL;
    ui_Sketchpad_ColorStatus = NULL;
    ui_Sketchpad_ClearBtn = NULL;
    ui_Sketchpad_ClearLabel = NULL;
    ui_Sketchpad_PointCount = 0;
    ui_Sketchpad_IsDrawing = false;
}
