#ifndef UI_PRIMLJENEPORUKE_H
#define UI_PRIMLJENEPORUKE_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_PrimljenePoruke_screen_init(void);
extern void ui_PrimljenePoruke_screen_destroy(void);
extern void ui_PrimljenePoruke_refresh(void);
extern lv_obj_t *ui_PrimljenePoruke;
extern lv_obj_t *ui_PrimljenePorukeBack;
extern lv_obj_t *ui_PrimljenePorukePanel;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
