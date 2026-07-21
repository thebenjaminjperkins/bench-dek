#ifndef MODULE_REGISTRY_H
#define MODULE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "capability_types.h"
#include "message-types/dek_capabilities.h"
#include "message-types/dek_descriptor.h"

#define MAX_REGISTERED_MODULES 8u
#define MAX_CAPABILITY_PROVIDERS 16u

typedef struct dek_capability_provider dek_capability_provider_t;
typedef struct dek_service_instance dek_service_instance_t;

typedef struct
{
    bool (*open)(
        dek_capability_provider_t *provider,
        const void *config,
        size_t config_length,
        dek_service_instance_t *instance);
    bool (*close)(
        dek_capability_provider_t *provider,
        dek_service_instance_t *instance);
} dek_capability_provider_api_t;

typedef struct
{
    bool occupied;
    dek_descriptor_payload_t descriptor;
} dek_module_record_t;

struct dek_capability_provider
{
    bool occupied;
    service_kind_t kind;
    dek_module_record_t *module;
    char capability_id[DEK_CAPABILITY_ID_MAX_LENGTH + 1u];
    uint16_t capability_version;
    dek_resource_policy_t resource_policy;
    void *driver_ctx;
    const dek_capability_provider_api_t *provider_api;
    const void *typed_api;
};

typedef struct
{
    dek_module_record_t modules[MAX_REGISTERED_MODULES];
    dek_capability_provider_t providers[MAX_CAPABILITY_PROVIDERS];
} module_registry_t;

bool module_registry_init(module_registry_t *registry);

dek_module_record_t *module_registry_register_module(
    module_registry_t *registry,
    const dek_descriptor_payload_t *descriptor);

bool module_registry_register_provider(
    module_registry_t *registry,
    dek_module_record_t *module,
    const char *capability_id,
    uint16_t capability_version,
    dek_resource_policy_t resource_policy,
    service_kind_t kind,
    void *driver_ctx,
    const dek_capability_provider_api_t *provider_api,
    const void *typed_api);

dek_module_record_t *module_registry_find_module(
    module_registry_t *registry,
    const uint8_t module_id[DEK_MODULE_ID_SIZE]);

dek_capability_provider_t *module_registry_find_provider(
    module_registry_t *registry,
    const char *capability_id,
    uint16_t capability_version,
    const uint8_t *preferred_module_id);

#endif
