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
#include "jpeg_decoder.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <string.h>

// Decoded at 1/2 JPEG scale → 80x60 (9.4 KB) fits in internal RAM.
// Use lv_img_set_zoom(512) to render at 2x = 160x120 on screen.
#define CAM_W  80
#define CAM_H  60
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

// --------------------------------------------------------------------------
// Public API – called from FrameReceiver task
// --------------------------------------------------------------------------
void ui_push_camera_frame(const uint8_t *jpeg_buf, size_t jpeg_len)
{
    if (!s_cam_buf || !jpeg_buf || jpeg_len == 0) {
        return;
    }

    esp_jpeg_image_cfg_t cfg = {
        .indata       = (uint8_t *)jpeg_buf,
        .indata_size  = (uint32_t)jpeg_len,
        .outbuf       = (uint8_t *)s_cam_buf,
        .outbuf_size  = CAM_W * CAM_H * sizeof(lv_color_t),
        .out_format   = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale    = JPEG_IMAGE_SCALE_1_2,  // 1/2 → 80x60 (fits without PSRAM)
        .flags        = { .swap_color_bytes = 1 }, // required for LV_COLOR_16_SWAP=1
    };

    esp_jpeg_image_output_t out_img;
    if (esp_jpeg_decode(&cfg, &out_img) == ESP_OK) {
        taskENTER_CRITICAL(&s_lock);
        s_cam_version++;
        taskEXIT_CRITICAL(&s_lock);
    } else {
        ESP_LOGW(TAG, "JPEG decode failed");
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
        // Canvas – decoded at 80x60, displayed at 2x zoom = 160x120
        s_canvas = lv_canvas_create(ui_Camera_Scr);
        lv_canvas_set_buffer(s_canvas, s_cam_buf, CAM_W, CAM_H, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_align(s_canvas, LV_ALIGN_CENTER);
        lv_obj_set_y(s_canvas, 15);
        lv_img_set_zoom(s_canvas, 512);  // 2x → 160x120 on screen

        // "Waiting for signal" overlay – hidden after first frame arrives
        s_no_signal = lv_label_create(ui_Camera_Scr);
        lv_obj_set_align(s_no_signal, LV_ALIGN_CENTER);
        lv_obj_set_y(s_no_signal, 15);
        lv_obj_set_style_text_color(s_no_signal, lv_color_hex(0x888888),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_no_signal, "Cekanje na sliku...");
    } else {
        // No buffer available – show static error
        s_no_signal = lv_label_create(ui_Camera_Scr);
        lv_obj_set_align(s_no_signal, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(s_no_signal, lv_color_hex(0xFF4444),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_no_signal, "Kamera nije dostupna\n(nema dovoljno RAM-a)");
    }

    lv_obj_add_event_cb(s_back_btn, s_event_back, LV_EVENT_ALL, NULL);

    // Reset displayed version so any pending frame is shown immediately
    s_displayed_version = s_cam_version - 1;

    s_refresh_timer = lv_timer_create(s_refresh_timer_cb, 50, NULL);
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
    // s_cam_buf intentionally kept – avoids re-alloc on next screen visit
}
