idf_component_register(
    SRCS
        "main.c"
        "host/app_main.c"
        "host/transport/dek_packet.c"
    INCLUDE_DIRS
        "."
        "host"
    REQUIRES esp_driver_tsens
)
