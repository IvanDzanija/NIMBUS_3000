#ifndef UI_CAMERA_GALLERY_H
#define UI_CAMERA_GALLERY_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ui_gallery_store_frame(const lv_color_t *src, uint16_t src_w, uint16_t src_h);
uint8_t ui_gallery_get_count(void);
bool ui_gallery_get_frame(uint8_t index, const lv_color_t **buf, uint16_t *w, uint16_t *h);

#ifdef __cplusplus
}
#endif

#endif
