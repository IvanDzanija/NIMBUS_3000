// Manual sketchpad screen

#ifndef UI_SKETCHPAD_SCR_H
#define UI_SKETCHPAD_SCR_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Sketchpad_Scr_screen_init(void);
extern void ui_Sketchpad_Scr_screen_destroy(void);
extern void ui_Sketchpad_Scr_refresh(void);
extern int ui_Sketchpad_render_gray8(uint8_t *buf, size_t len, int width, int height);
extern lv_obj_t *ui_Sketchpad_Scr;
extern lv_obj_t *ui_Sketchpad_Back;
extern lv_obj_t *ui_Sketchpad_BackImg;
extern lv_obj_t *ui_Sketchpad_Title;
extern lv_obj_t *ui_Sketchpad_Canvas;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
