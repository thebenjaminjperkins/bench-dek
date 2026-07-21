#include "module_registry.h"

#include <string.h>

static bool module_ids_match(
    const uint8_t left[DEK_MODULE_ID_SIZE],
    const uint8_t right[DEK_MODULE_ID_SIZE])
{
    return memcmp(left, right, DEK_MODULE_ID_SIZE) == 0;
}

static bool copy_capability_id(char *destination, const char *source)
{
    size_t i;

    if (destination == NULL || !dek_capability_id_is_valid(source))
    {
        return false;
    }

    for (i = 0; source[i] != '\0'; ++i)
    {
        destination[i] = source[i];
    }

    destination[i] = '\0';
    return true;
}

bool module_registry_init(module_registry_t *registry)
{
    if (registry == NULL)
    {
        return false;
    }

    memset(registry, 0, sizeof(*registry));
    return true;
}

dek_module_record_t *module_registry_register_module(
    module_registry_t *registry,
    const dek_descriptor_payload_t *descriptor)
{
    size_t i;

    if (registry == NULL || !dek_descriptor_payload_is_valid(descriptor))
    {
        return NULL;
    }

    for (i = 0; i < MAX_REGISTERED_MODULES; ++i)
    {
        if (registry->modules[i].occupied &&
            module_ids_match(registry->modules[i].descriptor.module_id, descriptor->module_id))
        {
            return NULL;
        }
    }

    for (i = 0; i < MAX_REGISTERED_MODULES; ++i)
    {
        if (!registry->modules[i].occupied)
        {
            registry->modules[i].occupied = true;
            registry->modules[i].descriptor = *descriptor;
            return &registry->modules[i];
        }
    }

    return NULL;
}

bool module_registry_register_provider(
    module_registry_t *registry,
    dek_module_record_t *module,
    const char *capability_id,
    uint16_t capability_version,
    dek_resource_policy_t resource_policy,
    service_kind_t kind,
    void *driver_ctx,
    const dek_capability_provider_api_t *provider_api,
    const void *typed_api)
{
    size_t i;

    if (registry == NULL ||
        module == NULL ||
        !module->occupied ||
        !dek_capability_id_is_valid(capability_id) ||
        capability_version == 0u ||
        !dek_resource_policy_is_valid((uint8_t)resource_policy) ||
        kind == SERVICE_KIND_NONE)
    {
        return false;
    }

    for (i = 0; i < MAX_CAPABILITY_PROVIDERS; ++i)
    {
        dek_capability_provider_t *provider = &registry->providers[i];

        if (provider->occupied &&
            provider->module == module &&
            provider->capability_version == capability_version &&
            strcmp(provider->capability_id, capability_id) == 0)
        {
            return false;
        }
    }

    for (i = 0; i < MAX_CAPABILITY_PROVIDERS; ++i)
    {
        dek_capability_provider_t *provider = &registry->providers[i];

        if (!provider->occupied)
        {
            memset(provider, 0, sizeof(*provider));
            provider->occupied = true;
            provider->kind = kind;
            provider->module = module;
            provider->capability_version = capability_version;
            provider->resource_policy = resource_policy;
            provider->driver_ctx = driver_ctx;
            provider->provider_api = provider_api;
            provider->typed_api = typed_api;
            if (!copy_capability_id(provider->capability_id, capability_id))
            {
                memset(provider, 0, sizeof(*provider));
                return false;
            }

            return true;
        }
    }

    return false;
}

dek_module_record_t *module_registry_find_module(
    module_registry_t *registry,
    const uint8_t module_id[DEK_MODULE_ID_SIZE])
{
    size_t i;

    if (registry == NULL || !dek_module_id_is_valid(module_id))
    {
        return NULL;
    }

    for (i = 0; i < MAX_REGISTERED_MODULES; ++i)
    {
        if (registry->modules[i].occupied &&
            module_ids_match(registry->modules[i].descriptor.module_id, module_id))
        {
            return &registry->modules[i];
        }
    }

    return NULL;
}

dek_capability_provider_t *module_registry_find_provider(
    module_registry_t *registry,
    const char *capability_id,
    uint16_t capability_version,
    const uint8_t *preferred_module_id)
{
    size_t i;

    if (registry == NULL ||
        !dek_capability_id_is_valid(capability_id) ||
        capability_version == 0u)
    {
        return NULL;
    }

    for (i = 0; i < MAX_CAPABILITY_PROVIDERS; ++i)
    {
        dek_capability_provider_t *provider = &registry->providers[i];

        if (!provider->occupied ||
            provider->capability_version != capability_version ||
            strcmp(provider->capability_id, capability_id) != 0)
        {
            continue;
        }

        if (preferred_module_id != NULL &&
            !module_ids_match(provider->module->descriptor.module_id, preferred_module_id))
        {
            continue;
        }

        return provider;
    }

    return NULL;
}
