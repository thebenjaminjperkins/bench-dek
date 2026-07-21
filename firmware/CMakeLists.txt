set(DEK_PROTOCOL_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/dek-protocol")

idf_component_register(
    SRCS
        "main.c"
        "host/app_main.c"
        "host/core/bootstrap.c"
        "host/applications/uart_smoke_app.c"
        "host/applications/spi_smoke_app.c"
        "host/service-api/service_api.c"
        "host/service-api/service-registry/service-registry.c"
        "host/service-api/services/service-gpio/service-gpio.c"
        "${DEK_PROTOCOL_DIR}/src/dek_crc.c"
        "${DEK_PROTOCOL_DIR}/src/dek_packet.c"
        "${DEK_PROTOCOL_DIR}/src/dek_transport.c"
        "${DEK_PROTOCOL_DIR}/src/message-types/dek_hello.c"
    INCLUDE_DIRS
        "."
        "host"
        "host/service-api"
        "${DEK_PROTOCOL_DIR}/include"
    REQUIRES 
        esp_driver_uart
        esp_driver_spi
)
