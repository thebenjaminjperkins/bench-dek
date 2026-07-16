idf_component_register(
    SRCS
        "main.c"
        "host/app_main.c"
    INCLUDE_DIRS
        "."
        "host"
    REQUIRES esp_driver_tsens
)
