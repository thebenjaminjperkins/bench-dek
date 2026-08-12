#include "bringup/gpio_vertical_slice_app.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "capabilities/dek_capability_ids.h"
#include "capabilities/dek_gpio_digital.h"
#include "dek_message.h"
#include "module-drivers/gpio_remote_provider.h"
#include "module-manager/module_registry.h"
#include "module-manager/service_instance.h"
#include "service-api/service_api.h"
#include "service-api/services/service-gpio/service-gpio.h"
#include "tests/fixtures/reference-gpio/reference_gpio_module.h"
#include "transport/host_transport_adapter.h"

static void gpio_vertical_slice_print_banner(void)
{
    printf("\n[BRINGUP] gpio_vertical_slice_app\n");
}

static bool gpio_vertical_slice_require(
    const char *step_name,
    bool condition,
    const char *failure_message)
{
    printf("  [gpio_vertical_slice] %s\n", step_name);

    if (!condition)
    {
        printf("    FAIL: %s\n", failure_message);
        return false;
    }

    printf("    OK\n");
    return true;
}

void gpio_vertical_slice_app_run(void)
{
    reference_gpio_module_t module;
    host_transport_adapter_t transport;
    gpio_remote_provider_driver_t provider_driver;
    module_registry_t registry;
    service_instance_registry_t instance_registry;
    service_api_t service_api;
    host_transport_hello_result_t hello_result;
    service_handle_t handle = { 0 };
    capability_request_t request;
    uint8_t value = 0u;
    bool handle_open = false;
    const uint8_t *preferred_module_id = NULL;

    gpio_vertical_slice_print_banner();

    if (!gpio_vertical_slice_require(
            "initialize reference module, transport, registries, and service API",
            reference_gpio_module_init(&module) &&
                host_transport_adapter_init(
                    &transport,
                    reference_gpio_module_exchange,
                    &module) &&
                gpio_remote_provider_driver_init(&provider_driver, &transport) &&
                module_registry_init(&registry) &&
                service_instance_registry_init(&instance_registry) &&
                service_api_init(&service_api, &registry, &instance_registry),
            "vertical slice runtime state should initialize"))
    {
        return;
    }

    if (!gpio_vertical_slice_require(
            "complete transport hello handshake",
            host_transport_adapter_send_hello(&transport, &hello_result) &&
                hello_result.selected_protocol_version == DEK_PROTOCOL_VERSION,
            "hello handshake should report protocol version 1"))
    {
        return;
    }

    if (!gpio_vertical_slice_require(
            "discover remote gpio capability and register provider",
            gpio_remote_provider_driver_discover_and_register(
                &provider_driver,
                &registry,
                NULL),
            "gpio capability should be discovered and registered"))
    {
        return;
    }

    preferred_module_id = gpio_remote_provider_driver_module_id(&provider_driver);
    memset(&request, 0, sizeof(request));
    request.capability_id = DEK_CAPABILITY_ID_GPIO_DIGITAL;
    request.capability_version = DEK_CAPABILITY_VERSION_GPIO_DIGITAL;
    request.preferred_module_id = preferred_module_id;
    request.lease_owner = 0x4750494Fu;

    if (!gpio_vertical_slice_require(
            "open gpio service through service_api",
            service_api_open(&service_api, &request, &handle),
            "service_api_open should produce a live gpio service handle"))
    {
        return;
    }

    handle_open = true;

    if (!gpio_vertical_slice_require(
            "set gpio pin 2 to output mode",
            gpio_service_set_mode(
                &handle,
                2u,
                DEK_GPIO_DIGITAL_MODE_OUTPUT),
            "gpio pin should enter output mode"))
    {
        goto cleanup;
    }

    if (!gpio_vertical_slice_require(
            "drive gpio pin 2 high",
            gpio_service_write(
                &handle,
                2u,
                DEK_GPIO_DIGITAL_LEVEL_HIGH),
            "gpio write high should succeed"))
    {
        goto cleanup;
    }

    if (!gpio_vertical_slice_require(
            "read gpio pin 2 high value back through service_api",
            gpio_service_read(&handle, 2u, &value) &&
                value == (uint8_t)DEK_GPIO_DIGITAL_LEVEL_HIGH,
            "gpio read should report high"))
    {
        goto cleanup;
    }

    if (!gpio_vertical_slice_require(
            "drive gpio pin 2 low",
            gpio_service_write(
                &handle,
                2u,
                DEK_GPIO_DIGITAL_LEVEL_LOW),
            "gpio write low should succeed"))
    {
        goto cleanup;
    }

    if (!gpio_vertical_slice_require(
            "read gpio pin 2 low value back through service_api",
            gpio_service_read(&handle, 2u, &value) &&
                value == (uint8_t)DEK_GPIO_DIGITAL_LEVEL_LOW,
            "gpio read should report low"))
    {
        goto cleanup;
    }

    printf("  [gpio_vertical_slice] PASS\n");

cleanup:
    if (handle_open)
    {
        if (service_api_close(&service_api, &handle))
        {
            printf("  [gpio_vertical_slice] close gpio service handle\n");
            printf("    OK\n");
        }
        else
        {
            printf("  [gpio_vertical_slice] close gpio service handle\n");
            printf("    FAIL: service_api_close should succeed\n");
        }
    }
}
