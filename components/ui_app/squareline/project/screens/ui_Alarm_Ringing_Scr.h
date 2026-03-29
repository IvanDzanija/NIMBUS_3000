// Manual alarm ringing screen

#ifndef UI_ALARM_RINGING_SCR_H
#define UI_ALARM_RINGING_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Alarm_Ringing_Scr_screen_init(void);
extern void ui_Alarm_Ringing_Scr_screen_destroy(void);
extern void ui_Alarm_Ringing_Scr_refresh(void);
extern lv_obj_t *ui_Alarm_Ringing_Scr;
extern lv_obj_t *ui_Alarm_Ringing_Title;
extern lv_obj_t *ui_Alarm_Ringing_Subtitle;
extern lv_obj_t *ui_Alarm_StopBtn;
extern lv_obj_t *ui_Alarm_StopLabel;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
