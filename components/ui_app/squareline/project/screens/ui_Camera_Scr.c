/**
 * @file ui_Camera_Scr.c
 * @brief Camera live-view screen.
 *
 * Receives raw JPEG frames from KidBoardReceiver via ui_push_camera_frame(),
 * decodes them to RGB565 using esp_jpeg_decode(), and renders them onto an
 * lv_canvas widget.  The decode/version-bump runs in the FrameReceiver task;
 * the LVGL canvas invalidation runs in the GUI task via an lv_timer.
 */

#include "../ui.h"
#include "ui_camera_gallery.h"
#include "jpeg_decoder.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <string.h>

// Decoded at 1/2 JPEG scale → 80x60 (9.4 KB) fits in internal RAM.
// Render the canvas enlarged in LVGL so the preview stays RAM-friendly.
#define CAM_W  80
#define CAM_H  60
#define CAM_ZOOM 768  // 3x -> 240x180 on screen
#define TAG    "CamScr"

// --------------------------------------------------------------------------
// LVGL objects
// --------------------------------------------------------------------------
lv_obj_t *ui_Camera_Scr  = NULL;
static lv_obj_t *s_canvas     = NULL;
static lv_obj_t *s_back_btn   = NULL;
static lv_obj_t *s_back_img   = NULL;
static lv_obj_t *s_title      = NULL;
static lv_obj_t *s_no_signal  = NULL;
static lv_obj_t *s_capture_btn = NULL;
static lv_obj_t *s_capture_label = NULL;
static lv_timer_t *s_refresh_timer = NULL;

// --------------------------------------------------------------------------
// Pixel buffer (allocated once in PSRAM, kept across screen destroy/init)
// --------------------------------------------------------------------------
static lv_color_t *s_cam_buf = NULL;   // CAM_W * CAM_H pixels

// --------------------------------------------------------------------------
// Cross-task synchronisation: FrameReceiver writes, LVGL timer reads
// --------------------------------------------------------------------------
static volatile uint32_t s_cam_version      = 0;
static          uint32_t s_displayed_version = 0;
static portMUX_TYPE      s_lock = portMUX_INITIALIZER_UNLOCKED;

static uint16_t s_scaled_dim(uint16_t value, uint8_t div)
{
    return (uint16_t)((value + div - 1) / div);
}

static bool s_try_scale_from_source(const esp_jpeg_image_output_t *src_info,
                                    esp_jpeg_image_scale_t scale,
                                    esp_jpeg_image_output_t *scaled_info)
{
    static const uint8_t s_scale_div[] = {1, 2, 4, 8};
    const uint8_t div = s_scale_div[scale];

    scaled_info->width = s_scaled_dim(src_info->width, div);
    scaled_info->height = s_scaled_dim(src_info->height, div);
    scaled_info->output_len = scaled_info->width * scaled_info->height * sizeof(uint16_t);

    return scaled_info->width <= CAM_W && scaled_info->height <= CAM_H &&
           scaled_info->output_len <= CAM_W * CAM_H * sizeof(lv_color_t);
}

static bool s_get_jpeg_source_info(const uint8_t *jpeg_buf, size_t jpeg_len,
                                   esp_jpeg_image_output_t *info)
{
    esp_jpeg_image_cfg_t cfg = {
        .indata      = (uint8_t *)jpeg_buf,
        .indata_size = (uint32_t)jpeg_len,
        .out_format  = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale   = JPEG_IMAGE_SCALE_0,
    };

    return esp_jpeg_get_image_info(&cfg, info) == ESP_OK;
}

void ui_Camera_Scr_refresh(void)
{
    const bool is_dark = ui_Dark_Mode_Switch && lv_obj_has_state(ui_Dark_Mode_Switch, LV_STATE_CHECKED);
    const lv_color_t screen_bg = is_dark ? lv_color_hex(0x111111) : lv_color_hex(0xFFF7F0);
    const lv_color_t text_color = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x111111);
    const lv_color_t hint_color = is_dark ? lv_color_hex(0x888888) : lv_color_hex(0x666666);
    const lv_color_t button_bg = is_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xEBA678);
    const lv_color_t button_text = lv_color_hex(0x111111);

    if (ui_Camera_Scr) {
        lv_obj_set_style_bg_color(ui_Camera_Scr, screen_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_title) {
        lv_obj_set_style_text_color(s_title, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_no_signal) {
        lv_obj_set_style_text_color(s_no_signal, s_cam_buf ? hint_color : lv_color_hex(0xB42318),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_capture_btn) {
        lv_obj_set_style_bg_color(s_capture_btn, button_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_capture_btn, is_dark ? 0 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(s_capture_btn, lv_color_hex(0xD08C5B),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (s_capture_label) {
        lv_obj_set_style_text_color(s_capture_label, button_text, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// --------------------------------------------------------------------------
// Public API – called from FrameReceiver task
// --------------------------------------------------------------------------
void ui_push_camera_frame(const uint8_t *jpeg_buf, size_t jpeg_len)
{
    if (!s_cam_buf || !jpeg_buf || jpeg_len == 0) {
        return;
    }

    esp_jpeg_image_output_t info = {0};
    esp_jpeg_image_output_t src_info = {0};
    esp_jpeg_image_scale_t scale = JPEG_IMAGE_SCALE_1_2;

    const bool has_src_info = s_get_jpeg_source_info(jpeg_buf, jpeg_len, &src_info);

    if (!has_src_info) {
        ESP_LOGW(TAG, "Failed to read JPEG header (len=%u)", (unsigned)jpeg_len);
        return;
    }

    if (s_try_scale_from_source(&src_info, JPEG_IMAGE_SCALE_1_2, &info)) {
        scale = JPEG_IMAGE_SCALE_1_2;
    } else if (s_try_scale_from_source(&src_info, JPEG_IMAGE_SCALE_1_4, &info)) {
        scale = JPEG_IMAGE_SCALE_1_4;
    } else if (s_try_scale_from_source(&src_info, JPEG_IMAGE_SCALE_1_8, &info)) {
        scale = JPEG_IMAGE_SCALE_1_8;
    } else {
        ESP_LOGW(TAG,
                 "JPEG frame too large for %dx%d canvas: src=%ux%u len=%u",
                 CAM_W, CAM_H, src_info.width, src_info.height, (unsigned)jpeg_len);
        return;
    }

    esp_jpeg_image_cfg_t cfg = {
        .indata       = (uint8_t *)jpeg_buf,
        .indata_size  = (uint32_t)jpeg_len,
        .outbuf       = (uint8_t *)s_cam_buf,
        .outbuf_size  = CAM_W * CAM_H * sizeof(lv_color_t),
        .out_format   = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale    = scale,
        .flags        = { .swap_color_bytes = 1 }, // required for LV_COLOR_16_SWAP=1
    };

    esp_jpeg_image_output_t out_img = {0};
    memset(s_cam_buf, 0, CAM_W * CAM_H * sizeof(lv_color_t));
    if (esp_jpeg_decode(&cfg, &out_img) == ESP_OK) {
        taskENTER_CRITICAL(&s_lock);
        s_cam_version++;
        taskEXIT_CRITICAL(&s_lock);
    } else {
        ESP_LOGW(TAG, "JPEG decode failed (len=%u, scale=%d)", (unsigned)jpeg_len, (int)scale);
    }
}

// --------------------------------------------------------------------------
// LVGL timer – runs inside lv_task_handler() (GUI task)
// --------------------------------------------------------------------------
static void s_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    uint32_t ver;
    taskENTER_CRITICAL(&s_lock);
    ver = s_cam_version;
    taskEXIT_CRITICAL(&s_lock);

    if (ver != s_displayed_version) {
        s_displayed_version = ver;
        if (s_canvas) {
            lv_obj_invalidate(s_canvas);
        }
        if (s_no_signal) {
            lv_obj_add_flag(s_no_signal, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// --------------------------------------------------------------------------
// Back button event
// --------------------------------------------------------------------------
static void s_event_back(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Home_Scr, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0,
                          &ui_Home_Scr_screen_init);
    }
}

void ui_camera_capture_current_frame(void)
{
    if (!s_cam_buf || s_cam_version == 0) {
        ESP_LOGW(TAG, "Capture skipped: no decoded frame yet");
        return;
    }

    if (ui_gallery_store_frame(s_cam_buf, CAM_W, CAM_H)) {
        ESP_LOGI(TAG, "Photo saved to gallery");
    } else {
        ESP_LOGW(TAG, "Photo save failed");
    }
}

static void s_event_capture(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Capture button pressed");
        ui_camera_capture_current_frame();
    }
}

// --------------------------------------------------------------------------
// Screen init / destroy
// --------------------------------------------------------------------------
void ui_Camera_Scr_screen_init(void)
{
    // Allocate pixel buffer once; keep it alive across re-inits
    if (!s_cam_buf) {
        s_cam_buf = heap_caps_malloc(CAM_W * CAM_H * sizeof(lv_color_t),
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_cam_buf) {
            ESP_LOGW(TAG, "PSRAM unavailable, falling back to internal RAM");
            s_cam_buf = malloc(CAM_W * CAM_H * sizeof(lv_color_t));
        }
        if (s_cam_buf) {
            memset(s_cam_buf, 0, CAM_W * CAM_H * sizeof(lv_color_t));
        } else {
            ESP_LOGE(TAG, "Failed to alloc cam buf — camera unavailable");
            // Fall through: screen is still created so LVGL never gets a NULL ptr
        }
    }

    // Root screen
    ui_Camera_Scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Camera_Scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Camera_Scr, lv_color_hex(0x111111),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Camera_Scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Back button (top-left, matches Sketchpad style)
    s_back_btn = lv_btn_create(ui_Camera_Scr);
    lv_obj_set_size(s_back_btn, 40, 30);
    lv_obj_set_x(s_back_btn, -124);
    lv_obj_set_y(s_back_btn, -96);
    lv_obj_set_align(s_back_btn, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(s_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_back_img = lv_img_create(s_back_btn);
    lv_img_set_src(s_back_img, &ui_img_back_button_png);
    lv_obj_set_align(s_back_img, LV_ALIGN_CENTER);
    lv_img_set_zoom(s_back_img, 110);

    // Screen title
    s_title = lv_label_create(ui_Camera_Scr);
    lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_title, 18);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_title, lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_title, "Kamera");

    if (s_cam_buf) {
        // Canvas – decoded at 80x60, displayed enlarged on screen.
        s_canvas = lv_canvas_create(ui_Camera_Scr);
        lv_canvas_set_buffer(s_canvas, s_cam_buf, CAM_W, CAM_H, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_align(s_canvas, LV_ALIGN_CENTER);
        lv_obj_set_y(s_canvas, 24);
        lv_img_set_zoom(s_canvas, CAM_ZOOM);

        // "Waiting for signal" overlay – hidden after first frame arrives
        s_no_signal = lv_label_create(ui_Camera_Scr);
        lv_obj_set_align(s_no_signal, LV_ALIGN_CENTER);
        lv_obj_set_y(s_no_signal, 24);
        lv_obj_set_style_text_color(s_no_signal, lv_color_hex(0x888888),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_no_signal, "Cekanje na sliku...");

        s_capture_btn = lv_btn_create(ui_Camera_Scr);
        lv_obj_set_size(s_capture_btn, 112, 42);
        lv_obj_set_align(s_capture_btn, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_y(s_capture_btn, -18);
        lv_obj_set_style_bg_color(s_capture_btn, lv_color_hex(0xFFFFFF),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(s_capture_btn, lv_color_hex(0x111111),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_capture_btn, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(s_capture_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_capture_label = lv_label_create(s_capture_btn);
        lv_label_set_text(s_capture_label, "Snimi");
        lv_obj_set_style_text_font(s_capture_label, &lv_font_montserrat_16,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(s_capture_label);
    } else {
        // No buffer available – show static error
        s_no_signal = lv_label_create(ui_Camera_Scr);
        lv_obj_set_align(s_no_signal, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(s_no_signal, lv_color_hex(0xFF4444),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_no_signal, "Kamera nije dostupna\n(nema dovoljno RAM-a)");
    }

    lv_obj_add_event_cb(s_back_btn, s_event_back, LV_EVENT_ALL, NULL);
    if (s_capture_btn) {
        lv_obj_add_event_cb(s_capture_btn, s_event_capture, LV_EVENT_ALL, NULL);
    }

    // Reset displayed version so any pending frame is shown immediately
    s_displayed_version = s_cam_version - 1;

    s_refresh_timer = lv_timer_create(s_refresh_timer_cb, 50, NULL);
    ui_Camera_Scr_refresh();
}

void ui_Camera_Scr_screen_destroy(void)
{
    if (s_refresh_timer) {
        lv_timer_del(s_refresh_timer);
        s_refresh_timer = NULL;
    }
    if (ui_Camera_Scr) {
        lv_obj_del(ui_Camera_Scr);
    }
    ui_Camera_Scr = NULL;
    s_canvas      = NULL;
    s_back_btn    = NULL;
    s_back_img    = NULL;
    s_title       = NULL;
    s_no_signal   = NULL;
    s_capture_btn = NULL;
    s_capture_label = NULL;
    // s_cam_buf intentionally kept – avoids re-alloc on next screen visit
}
