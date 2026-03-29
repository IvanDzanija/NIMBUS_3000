set(COMPONENT_SRCS
    "APDS_9960.c"
    "gui.c"
    "i2c_manager.c"
    "app_main.cpp"
    "max98357a.c"
    "potentiometer.c"
    "sensors.c"
    "veml7700.c"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_EMBED_FILES
    "../sounds/dog.pcm"
    "../sounds/cat.pcm"
    "../sounds/cow.pcm"
    "../sounds/pig.pcm"
    "../sounds/horse.pcm"
    "../sounds/sealion.pcm"
    "../sounds/message.pcm"
)

register_component()
