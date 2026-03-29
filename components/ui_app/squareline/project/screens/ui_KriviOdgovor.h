// Manual screen for animal game result

#ifndef UI_KRIVIODGOVOR_H
#define UI_KRIVIODGOVOR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_KriviOdgovor_screen_init(void);
extern void ui_KriviOdgovor_screen_destroy(void);
extern void ui_KriviOdgovor_refresh(void);
extern lv_obj_t *ui_KriviOdgovor;
extern lv_obj_t *ui_KriviBack;
extern lv_obj_t *ui_KriviBackImg;
extern lv_obj_t *ui_KriviTitle;
extern lv_obj_t *ui_KriviSubtitle;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
