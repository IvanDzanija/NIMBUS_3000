#ifndef UI_CAMERA_SCR_H
#define UI_CAMERA_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// SCREEN: ui_Camera_Scr
extern lv_obj_t *ui_Camera_Scr;
void ui_Camera_Scr_screen_init(void);
void ui_Camera_Scr_screen_destroy(void);
void ui_Camera_Scr_refresh(void);

// Push a JPEG frame from the FrameReceiver task to the camera canvas.
// Safe to call from any FreeRTOS task.
void ui_push_camera_frame(const uint8_t *jpeg_buf, size_t jpeg_len);
void ui_camera_capture_current_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_CAMERA_SCR_H */
