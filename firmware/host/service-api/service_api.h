#ifndef SERVICE_API_H
#define SERVICE_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "module-manager/module_registry.h"
#include "module-manager/service_instance.h"
#include "service-api/services/services.h"

typedef struct
{
    const char *capability_id;
    uint16_t capability_version;
    const uint8_t *preferred_module_id;
    const void *config;
    size_t config_length;
    uint32_t lease_owner;
} capability_request_t;

typedef struct
{
    module_registry_t *module_registry;
    service_instance_registry_t *instance_registry;
} service_api_t;

bool service_api_init(
    service_api_t *api,
    module_registry_t *module_registry,
    service_instance_registry_t *instance_registry);

bool service_api_open(
    service_api_t *api,
    const capability_request_t *request,
    service_handle_t *out_handle);

bool service_api_close(
    service_api_t *api,
    service_handle_t *handle);

bool service_api_get_instance(
    const service_handle_t *handle,
    const dek_service_instance_t **out_instance);

#endif
