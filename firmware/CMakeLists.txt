set(DEK_PROTOCOL_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/DeK-Protocol")
set(DEK_PROTOCOL_ROOT "${DEK_PROTOCOL_DIR}")

include("${DEK_PROTOCOL_ROOT}/cmake/dek_protocol_sources.cmake")

idf_component_register(
    SRCS
        "main.c"
        "host/app_main.c"
        "host/core/bootstrap.c"
        "host/bringup/dev_runtime.c"
        "host/bringup/smoke/uart_smoke_app.c"
        "host/bringup/smoke/spi_smoke_app.c"
        "host/tests/module_service_unit_test.c"
        "host/service-api/service_api.c"
        "host/service-api/services/service-gpio/service-gpio.c"
        "host/module-manager/module_registry.c"
        "host/module-manager/service_instance.c"
        ${DEK_PROTOCOL_SOURCES}
    INCLUDE_DIRS
        "."
        "host"
        "host/service-api"
        "${DEK_PROTOCOL_INCLUDE_DIR}"
    REQUIRES 
        esp_driver_uart
        esp_driver_spi
)
