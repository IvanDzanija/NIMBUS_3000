// Manual screen for animal game result

#ifndef UI_TOCANODGOVOR_H
#define UI_TOCANODGOVOR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_TocanOdgovor_screen_init(void);
extern void ui_TocanOdgovor_screen_destroy(void);
extern void ui_TocanOdgovor_refresh(void);
extern lv_obj_t *ui_TocanOdgovor;
extern lv_obj_t *ui_TocanBack;
extern lv_obj_t *ui_TocanBackImg;
extern lv_obj_t *ui_TocanTitle;
extern lv_obj_t *ui_TocanSubtitle;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
