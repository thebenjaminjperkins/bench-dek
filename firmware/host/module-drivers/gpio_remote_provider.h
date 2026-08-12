#ifndef GPIO_REMOTE_PROVIDER_H
#define GPIO_REMOTE_PROVIDER_H

#include <stdbool.h>
#include <stdint.h>

#include "module-manager/module_registry.h"
#include "transport/host_transport_adapter.h"

typedef struct
{
    host_transport_adapter_t *transport;
    uint8_t module_id[DEK_MODULE_ID_SIZE];
    bool module_id_valid;
} gpio_remote_provider_driver_t;

bool gpio_remote_provider_driver_init(
    gpio_remote_provider_driver_t *driver,
    host_transport_adapter_t *transport);

bool gpio_remote_provider_driver_discover_and_register(
    gpio_remote_provider_driver_t *driver,
    module_registry_t *registry,
    dek_module_record_t **out_module_record);

const uint8_t *gpio_remote_provider_driver_module_id(
    const gpio_remote_provider_driver_t *driver);

#endif
