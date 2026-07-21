#include "service_instance.h"

#include <string.h>

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

bool service_instance_registry_init(service_instance_registry_t *registry)
{
    if (registry == NULL)
    {
        return false;
    }

    memset(registry, 0, sizeof(*registry));
    registry->next_instance_id = 1u;
    return true;
}

dek_service_instance_t *service_instance_registry_allocate(
    service_instance_registry_t *registry,
    dek_capability_provider_t *provider,
    const char *capability_id,
    uint16_t capability_version,
    const void *config,
    size_t config_length,
    uint32_t lease_owner)
{
    size_t i;
    dek_service_instance_t *instance;

    if (registry == NULL ||
        provider == NULL ||
        !provider->occupied ||
        !dek_capability_id_is_valid(capability_id) ||
        capability_version == 0u ||
        (config == NULL && config_length > 0u) ||
        config_length > SERVICE_INSTANCE_CONFIG_MAX)
    {
        return NULL;
    }

    for (i = 0; i < MAX_SERVICE_INSTANCES; ++i)
    {
        if (!registry->instances[i].occupied)
        {
            instance = &registry->instances[i];
            memset(instance, 0, sizeof(*instance));
            instance->occupied = true;
            instance->instance_id = registry->next_instance_id++;
            if (registry->next_instance_id == 0u)
            {
                registry->next_instance_id = 1u;
            }

            instance->kind = provider->kind;
            instance->provider = provider;
            instance->typed_api = provider->typed_api;
            instance->capability_version = capability_version;
            instance->state = SERVICE_INSTANCE_STATE_ALLOCATED;
            instance->lease_owner = lease_owner;
            instance->config_length = (uint16_t)config_length;

            if (!copy_capability_id(instance->capability_id, capability_id))
            {
                memset(instance, 0, sizeof(*instance));
                return NULL;
            }

            if (config != NULL && config_length > 0u)
            {
                memcpy(instance->config, config, config_length);
            }

            return instance;
        }
    }

    return NULL;
}

dek_service_instance_t *service_instance_registry_find_by_id(
    service_instance_registry_t *registry,
    uint32_t instance_id)
{
    size_t i;

    if (registry == NULL || instance_id == 0u)
    {
        return NULL;
    }

    for (i = 0; i < MAX_SERVICE_INSTANCES; ++i)
    {
        if (registry->instances[i].occupied &&
            registry->instances[i].instance_id == instance_id)
        {
            return &registry->instances[i];
        }
    }

    return NULL;
}

dek_service_instance_t *service_instance_registry_find_by_provider(
    service_instance_registry_t *registry,
    const dek_capability_provider_t *provider)
{
    size_t i;

    if (registry == NULL || provider == NULL)
    {
        return NULL;
    }

    for (i = 0; i < MAX_SERVICE_INSTANCES; ++i)
    {
        if (registry->instances[i].occupied &&
            registry->instances[i].provider == provider)
        {
            return &registry->instances[i];
        }
    }

    return NULL;
}

bool service_instance_registry_release(
    service_instance_registry_t *registry,
    dek_service_instance_t *instance)
{
    (void)registry;

    if (instance == NULL || !instance->occupied)
    {
        return false;
    }

    memset(instance, 0, sizeof(*instance));
    return true;
}
