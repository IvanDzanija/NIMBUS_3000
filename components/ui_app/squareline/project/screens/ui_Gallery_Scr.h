#ifndef UI_GALLERY_SCR_H
#define UI_GALLERY_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_Gallery_Scr;
void ui_Gallery_Scr_screen_init(void);
void ui_Gallery_Scr_screen_destroy(void);
void ui_Gallery_Scr_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
