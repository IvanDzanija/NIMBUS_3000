SET(SOURCES screens/ui_Home_Scr.c
    screens/ui_Alarm_Scr.c
    screens/ui_Sketchpad_Scr.c
    screens/ui_Alarm_Ringing_Scr.c
    screens/ui_Color_Scr.c
    screens/ui_Settings_Scr.c
    screens/ui_Whats_Down.c
    screens/ui_MamaKontakt.c
    screens/ui_TataKontakt.c
    screens/ui_PrimljenePoruke.c
    screens/ui_Zivotinje.c
    screens/ui_PogodiKojaJeZivotinja.c
    screens/ui_TocanOdgovor.c
    screens/ui_KriviOdgovor.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_colorwheel_icon_png.c
    images/ui_img_bl_logo_symbol_png.c
    images/ui_img_settings_icon_png.c
    images/ui_img_sketchpad_icon_png.c
    images/ui_img_images_png.c
    images/ui_img_animals_png.c
    images/ui_img_home_icon_png.c
    images/ui_img_plus_icon_png.c
    images/ui_img_back_button_png.c
    images/ui_img_dog_png.c
    images/ui_img_cat_png.c
    images/ui_img_cow_png.c
    images/ui_img_pig_png.c
    images/ui_img_horse_png.c
    images/ui_img_seal_png.c
    images/ui_img_speaker_png.c)

add_library(ui ${SOURCES})
