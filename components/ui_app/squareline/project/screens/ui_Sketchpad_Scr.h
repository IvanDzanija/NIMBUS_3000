// Manual sketchpad screen

#ifndef UI_SKETCHPAD_SCR_H
#define UI_SKETCHPAD_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Sketchpad_Scr_screen_init(void);
extern void ui_Sketchpad_Scr_screen_destroy(void);
extern void ui_Sketchpad_Scr_refresh(void);
extern lv_obj_t *ui_Sketchpad_Scr;
extern lv_obj_t *ui_Sketchpad_Back;
extern lv_obj_t *ui_Sketchpad_BackImg;
extern lv_obj_t *ui_Sketchpad_Title;
extern lv_obj_t *ui_Sketchpad_Canvas;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
