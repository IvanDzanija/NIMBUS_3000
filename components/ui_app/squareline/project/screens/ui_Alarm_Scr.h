// Manual alarm settings screen

#ifndef UI_ALARM_SCR_H
#define UI_ALARM_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Alarm_Scr_screen_init(void);
extern void ui_Alarm_Scr_screen_destroy(void);
extern void ui_Alarm_Scr_refresh(void);
extern lv_obj_t *ui_Alarm_Scr;
extern lv_obj_t *ui_Alarm_Back;
extern lv_obj_t *ui_Alarm_BackImg;
extern lv_obj_t *ui_Alarm_Title;
extern lv_obj_t *ui_Alarm_HourRoller;
extern lv_obj_t *ui_Alarm_MinuteRoller;
extern lv_obj_t *ui_Alarm_Colon;
extern lv_obj_t *ui_Alarm_EnableSwitch;
extern lv_obj_t *ui_Alarm_EnableLabel;
extern lv_obj_t *ui_Alarm_SaveBtn;
extern lv_obj_t *ui_Alarm_SaveLabel;
extern lv_obj_t *ui_Alarm_Status;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
