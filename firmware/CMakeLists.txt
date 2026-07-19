idf_component_register(
    SRCS
        "main.c"
        "host/app_main.c"
        "host/transport/dek_packet.c"
        "host/transport/dek_transport.c"
        "host/transport/dek_crc.c"
        "host/transport/messages/dek_hello.c"
    INCLUDE_DIRS
        "."
        "host"
        "host/transport"
    REQUIRES esp_driver_tsens
)
