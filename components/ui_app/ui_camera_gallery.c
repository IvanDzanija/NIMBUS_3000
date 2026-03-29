#include "ui_camera_gallery.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#define GALLERY_COUNT 5
#define THUMB_W 40
#define THUMB_H 30
#define TAG "Gallery"

static lv_color_t *s_thumb_bufs[GALLERY_COUNT];
static uint8_t s_gallery_count = 0;
static uint8_t s_next_slot = 0;

static bool s_ensure_buffers(void)
{
    for (uint8_t i = 0; i < GALLERY_COUNT; ++i) {
        if (s_thumb_bufs[i]) {
            continue;
        }

        s_thumb_bufs[i] = heap_caps_malloc(THUMB_W * THUMB_H * sizeof(lv_color_t),
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_thumb_bufs[i]) {
            s_thumb_bufs[i] = malloc(THUMB_W * THUMB_H * sizeof(lv_color_t));
        }
        if (!s_thumb_bufs[i]) {
            ESP_LOGE(TAG, "Failed to allocate thumbnail buffer %u", (unsigned)i);
            return false;
        }
    }

    return true;
}

static uint8_t s_slot_for_index(uint8_t index)
{
    return (uint8_t)((s_next_slot + GALLERY_COUNT - 1 - index) % GALLERY_COUNT);
}

bool ui_gallery_store_frame(const lv_color_t *src, uint16_t src_w, uint16_t src_h)
{
    if (!src || src_w == 0 || src_h == 0 || !s_ensure_buffers()) {
        ESP_LOGW(TAG, "Store failed: src=%p size=%ux%u", src, src_w, src_h);
        return false;
    }

    lv_color_t *dst = s_thumb_bufs[s_next_slot];
    for (uint16_t y = 0; y < THUMB_H; ++y) {
        const uint16_t src_y = (uint16_t)((y * src_h) / THUMB_H);
        for (uint16_t x = 0; x < THUMB_W; ++x) {
            const uint16_t src_x = (uint16_t)((x * src_w) / THUMB_W);
            dst[y * THUMB_W + x] = src[src_y * src_w + src_x];
        }
    }

    s_next_slot = (uint8_t)((s_next_slot + 1) % GALLERY_COUNT);
    if (s_gallery_count < GALLERY_COUNT) {
        s_gallery_count++;
    }

    ESP_LOGI(TAG, "Stored photo: slot=%u count=%u src=%ux%u",
             (unsigned)((s_next_slot + GALLERY_COUNT - 1) % GALLERY_COUNT),
             (unsigned)s_gallery_count, src_w, src_h);

    return true;
}

uint8_t ui_gallery_get_count(void)
{
    return s_gallery_count;
}

bool ui_gallery_get_frame(uint8_t index, const lv_color_t **buf, uint16_t *w, uint16_t *h)
{
    if (!buf || !w || !h || index >= s_gallery_count) {
        return false;
    }

    *buf = s_thumb_bufs[s_slot_for_index(index)];
    *w = THUMB_W;
    *h = THUMB_H;
    return true;
}
