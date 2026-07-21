#include "service_api.h"

#include <string.h>

#include "capabilities/dek_capability_ids.h"

static service_kind_t service_kind_from_capability(
    const char *capability_id,
    uint16_t capability_version)
{
    if (capability_id == NULL || capability_version == 0u)
    {
        return SERVICE_KIND_NONE;
    }

    if (capability_version == DEK_CAPABILITY_VERSION_UART_STREAM &&
        strcmp(capability_id, DEK_CAPABILITY_ID_UART_STREAM) == 0)
    {
        return SERVICE_KIND_UART_STREAM;
    }

    if (capability_version == DEK_CAPABILITY_VERSION_GPIO_DIGITAL &&
        strcmp(capability_id, DEK_CAPABILITY_ID_GPIO_DIGITAL) == 0)
    {
        return SERVICE_KIND_GPIO_DIGITAL;
    }

    return SERVICE_KIND_NONE;
}

bool service_api_init(
    service_api_t *api,
    module_registry_t *module_registry,
    service_instance_registry_t *instance_registry)
{
    if (api == NULL || module_registry == NULL || instance_registry == NULL)
    {
        return false;
    }

    api->module_registry = module_registry;
    api->instance_registry = instance_registry;
    return true;
}

bool service_api_open(
    service_api_t *api,
    const capability_request_t *request,
    service_handle_t *out_handle)
{
    dek_capability_provider_t *provider;
    dek_service_instance_t *instance;
    service_kind_t expected_kind;

    if (api == NULL ||
        api->module_registry == NULL ||
        api->instance_registry == NULL ||
        request == NULL ||
        out_handle == NULL ||
        !dek_capability_id_is_valid(request->capability_id) ||
        (request->config == NULL && request->config_length > 0u) ||
        request->capability_version == 0u)
    {
        return false;
    }

    expected_kind = service_kind_from_capability(
        request->capability_id,
        request->capability_version);
    if (expected_kind == SERVICE_KIND_NONE)
    {
        return false;
    }

    provider = module_registry_find_provider(
        api->module_registry,
        request->capability_id,
        request->capability_version,
        request->preferred_module_id);
    if (provider == NULL || provider->kind != expected_kind)
    {
        return false;
    }

    if (provider->resource_policy != DEK_RESOURCE_POLICY_SHARED &&
        service_instance_registry_find_by_provider(api->instance_registry, provider) != NULL)
    {
        return false;
    }

    instance = service_instance_registry_allocate(
        api->instance_registry,
        provider,
        request->capability_id,
        request->capability_version,
        request->config,
        request->config_length,
        request->lease_owner);
    if (instance == NULL)
    {
        return false;
    }

    if (provider->provider_api != NULL &&
        provider->provider_api->open != NULL &&
        !provider->provider_api->open(
            provider,
            request->config,
            request->config_length,
            instance))
    {
        (void)service_instance_registry_release(api->instance_registry, instance);
        return false;
    }

    if (instance->state == SERVICE_INSTANCE_STATE_ALLOCATED)
    {
        instance->state = SERVICE_INSTANCE_STATE_OPEN;
    }

    out_handle->instance_id = instance->instance_id;
    out_handle->kind = instance->kind;
    out_handle->instance = instance;
    out_handle->api = instance->typed_api;
    return true;
}

bool service_api_close(
    service_api_t *api,
    service_handle_t *handle)
{
    dek_service_instance_t *instance;

    if (api == NULL ||
        api->instance_registry == NULL ||
        !service_handle_is_bound(handle))
    {
        return false;
    }

    instance = service_instance_registry_find_by_id(
        api->instance_registry,
        handle->instance_id);
    if (instance == NULL || instance != handle->instance)
    {
        return false;
    }

    if (instance->provider != NULL &&
        instance->provider->provider_api != NULL &&
        instance->provider->provider_api->close != NULL &&
        !instance->provider->provider_api->close(instance->provider, instance))
    {
        return false;
    }

    if (!service_instance_registry_release(api->instance_registry, instance))
    {
        return false;
    }

    memset(handle, 0, sizeof(*handle));
    return true;
}

bool service_api_get_instance(
    const service_handle_t *handle,
    const dek_service_instance_t **out_instance)
{
    if (!service_handle_is_bound(handle) || out_instance == NULL)
    {
        return false;
    }

    if (handle->instance->instance_id != handle->instance_id)
    {
        return false;
    }

    *out_instance = handle->instance;
    return true;
}
