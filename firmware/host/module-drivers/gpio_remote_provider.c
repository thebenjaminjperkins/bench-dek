#include "module-drivers/gpio_remote_provider.h"

#include <string.h>

#include "capabilities/dek_capability_ids.h"
#include "capabilities/dek_gpio_digital.h"
#include "message-types/dek_service_command.h"
#include "module-manager/service_instance.h"
#include "service-api/services/service-gpio/service-gpio.h"

static bool gpio_remote_provider_open(
    dek_capability_provider_t *provider,
    const void *config,
    size_t config_length,
    dek_service_instance_t *instance);
static bool gpio_remote_provider_close(
    dek_capability_provider_t *provider,
    dek_service_instance_t *instance);
static bool gpio_remote_provider_set_mode(
    dek_service_instance_t *instance,
    uint8_t pin,
    dek_gpio_digital_pin_mode_t mode);
static bool gpio_remote_provider_write(
    dek_service_instance_t *instance,
    uint8_t pin,
    uint8_t value);
static bool gpio_remote_provider_read(
    dek_service_instance_t *instance,
    uint8_t pin,
    uint8_t *value);

static const dek_capability_provider_api_t s_gpio_remote_provider_api = {
    .open = gpio_remote_provider_open,
    .close = gpio_remote_provider_close,
};

static const gpio_service_api_t s_gpio_remote_service_api = {
    .set_mode = gpio_remote_provider_set_mode,
    .write = gpio_remote_provider_write,
    .read = gpio_remote_provider_read,
};

static bool gpio_remote_provider_manifest_contains_gpio(
    const uint8_t *manifest,
    uint16_t manifest_length,
    dek_resource_policy_t *out_policy)
{
    size_t offset = 0u;

    if (manifest == NULL)
    {
        return false;
    }

    while (offset < manifest_length)
    {
        dek_capability_manifest_entry_header_t entry;
        size_t entry_length;
        char capability_id[DEK_CAPABILITY_ID_MAX_LENGTH + 1u];

        if (manifest_length - offset < sizeof(entry))
        {
            return false;
        }

        memcpy(&entry, manifest + offset, sizeof(entry));
        entry_length = sizeof(entry) +
                       (size_t)entry.capability_id_length +
                       (size_t)entry.limits_blob_length;
        if (!dek_capability_manifest_entry_header_is_valid(
                &entry,
                manifest_length - offset) ||
            manifest_length - offset < entry_length)
        {
            return false;
        }

        memcpy(
            capability_id,
            manifest + offset + sizeof(entry),
            entry.capability_id_length);
        capability_id[entry.capability_id_length] = '\0';

        if (entry.capability_version == DEK_CAPABILITY_VERSION_GPIO_DIGITAL &&
            strcmp(capability_id, DEK_CAPABILITY_ID_GPIO_DIGITAL) == 0)
        {
            if (out_policy != NULL)
            {
                *out_policy = (dek_resource_policy_t)entry.resource_policy;
            }

            return true;
        }

        offset += entry_length;
    }

    return false;
}

static gpio_remote_provider_driver_t *gpio_remote_provider_driver_from_provider(
    dek_capability_provider_t *provider)
{
    if (provider == NULL || provider->driver_ctx == NULL)
    {
        return NULL;
    }

    return (gpio_remote_provider_driver_t *)provider->driver_ctx;
}

static gpio_remote_provider_driver_t *gpio_remote_provider_driver_from_instance(
    dek_service_instance_t *instance)
{
    if (instance == NULL || instance->provider == NULL)
    {
        return NULL;
    }

    return gpio_remote_provider_driver_from_provider(instance->provider);
}

bool gpio_remote_provider_driver_init(
    gpio_remote_provider_driver_t *driver,
    host_transport_adapter_t *transport)
{
    if (driver == NULL || transport == NULL)
    {
        return false;
    }

    memset(driver, 0, sizeof(*driver));
    driver->transport = transport;
    return true;
}

bool gpio_remote_provider_driver_discover_and_register(
    gpio_remote_provider_driver_t *driver,
    module_registry_t *registry,
    dek_module_record_t **out_module_record)
{
    dek_descriptor_payload_t descriptor;
    uint8_t manifest[HOST_TRANSPORT_ADAPTER_CAPABILITY_MAX];
    dek_resource_policy_t resource_policy = DEK_RESOURCE_POLICY_SHARED;
    uint16_t manifest_total_bytes = 0u;
    uint16_t chunk_offset = 0u;
    uint16_t chunk_length = 0u;
    dek_module_record_t *module_record;

    if (driver == NULL || driver->transport == NULL || registry == NULL)
    {
        return false;
    }

    if (!host_transport_adapter_get_descriptor(driver->transport, &descriptor))
    {
        return false;
    }

    if (!host_transport_adapter_get_capabilities(
            driver->transport,
            0u,
            sizeof(manifest),
            manifest,
            sizeof(manifest),
            &manifest_total_bytes,
            &chunk_offset,
            &chunk_length))
    {
        return false;
    }

    if (chunk_offset != 0u ||
        chunk_length != manifest_total_bytes ||
        !gpio_remote_provider_manifest_contains_gpio(
            manifest,
            chunk_length,
            &resource_policy))
    {
        return false;
    }

    module_record = module_registry_register_module(registry, &descriptor);
    if (module_record == NULL)
    {
        module_record = module_registry_find_module(registry, descriptor.module_id);
        if (module_record == NULL)
        {
            return false;
        }
    }

    memcpy(driver->module_id, descriptor.module_id, sizeof(driver->module_id));
    driver->module_id_valid = true;

    if (!module_registry_register_provider(
            registry,
            module_record,
            DEK_CAPABILITY_ID_GPIO_DIGITAL,
            DEK_CAPABILITY_VERSION_GPIO_DIGITAL,
            resource_policy,
            SERVICE_KIND_GPIO_DIGITAL,
            driver,
            &s_gpio_remote_provider_api,
            &s_gpio_remote_service_api))
    {
        dek_capability_provider_t *provider = module_registry_find_provider(
            registry,
            DEK_CAPABILITY_ID_GPIO_DIGITAL,
            DEK_CAPABILITY_VERSION_GPIO_DIGITAL,
            descriptor.module_id);
        if (provider == NULL)
        {
            return false;
        }
    }

    if (out_module_record != NULL)
    {
        *out_module_record = module_record;
    }

    return true;
}

const uint8_t *gpio_remote_provider_driver_module_id(
    const gpio_remote_provider_driver_t *driver)
{
    if (driver == NULL || !driver->module_id_valid)
    {
        return NULL;
    }

    return driver->module_id;
}

static bool gpio_remote_provider_open(
    dek_capability_provider_t *provider,
    const void *config,
    size_t config_length,
    dek_service_instance_t *instance)
{
    gpio_remote_provider_driver_t *driver =
        gpio_remote_provider_driver_from_provider(provider);
    host_transport_open_result_t open_result;

    if (driver == NULL ||
        driver->transport == NULL ||
        instance == NULL ||
        config_length > UINT16_MAX)
    {
        return false;
    }

    if (!host_transport_adapter_open_capability(
            driver->transport,
            provider->capability_id,
            provider->capability_version,
            config,
            (uint16_t)config_length,
            &open_result))
    {
        return false;
    }

    instance->channel_id = open_result.channel_id;
    return true;
}

static bool gpio_remote_provider_close(
    dek_capability_provider_t *provider,
    dek_service_instance_t *instance)
{
    gpio_remote_provider_driver_t *driver =
        gpio_remote_provider_driver_from_provider(provider);

    if (driver == NULL ||
        driver->transport == NULL ||
        instance == NULL ||
        instance->channel_id == 0u)
    {
        return false;
    }

    return host_transport_adapter_close_capability(
        driver->transport,
        instance->channel_id);
}

static bool gpio_remote_provider_set_mode(
    dek_service_instance_t *instance,
    uint8_t pin,
    dek_gpio_digital_pin_mode_t mode)
{
    gpio_remote_provider_driver_t *driver =
        gpio_remote_provider_driver_from_instance(instance);
    dek_gpio_digital_set_mode_request_t request;
    dek_gpio_digital_set_mode_response_t response;
    uint8_t status = 0u;
    uint16_t response_length = 0u;

    if (driver == NULL || driver->transport == NULL)
    {
        return false;
    }

    memset(&request, 0, sizeof(request));
    request.pin = pin;
    request.mode = (uint8_t)mode;

    if (!host_transport_adapter_send_command(
            driver->transport,
            instance->channel_id,
            (uint8_t)DEK_GPIO_DIGITAL_CMD_SET_MODE,
            &request,
            sizeof(request),
            &status,
            &response,
            sizeof(response),
            &response_length))
    {
        return false;
    }

    return status == (uint8_t)DEK_SERVICE_COMMAND_STATUS_OK &&
           response_length == sizeof(response) &&
           response.pin == pin &&
           response.applied_mode == (uint8_t)mode;
}

static bool gpio_remote_provider_write(
    dek_service_instance_t *instance,
    uint8_t pin,
    uint8_t value)
{
    gpio_remote_provider_driver_t *driver =
        gpio_remote_provider_driver_from_instance(instance);
    dek_gpio_digital_write_request_t request;
    dek_gpio_digital_write_response_t response;
    uint8_t status = 0u;
    uint16_t response_length = 0u;

    if (driver == NULL || driver->transport == NULL)
    {
        return false;
    }

    memset(&request, 0, sizeof(request));
    request.pin = pin;
    request.value = value;

    if (!host_transport_adapter_send_command(
            driver->transport,
            instance->channel_id,
            (uint8_t)DEK_GPIO_DIGITAL_CMD_WRITE,
            &request,
            sizeof(request),
            &status,
            &response,
            sizeof(response),
            &response_length))
    {
        return false;
    }

    return status == (uint8_t)DEK_SERVICE_COMMAND_STATUS_OK &&
           response_length == sizeof(response) &&
           response.pin == pin &&
           response.applied_value == value;
}

static bool gpio_remote_provider_read(
    dek_service_instance_t *instance,
    uint8_t pin,
    uint8_t *value)
{
    gpio_remote_provider_driver_t *driver =
        gpio_remote_provider_driver_from_instance(instance);
    dek_gpio_digital_read_request_t request;
    dek_gpio_digital_read_response_t response;
    uint8_t status = 0u;
    uint16_t response_length = 0u;

    if (driver == NULL || driver->transport == NULL || value == NULL)
    {
        return false;
    }

    memset(&request, 0, sizeof(request));
    request.pin = pin;

    if (!host_transport_adapter_send_command(
            driver->transport,
            instance->channel_id,
            (uint8_t)DEK_GPIO_DIGITAL_CMD_READ,
            &request,
            sizeof(request),
            &status,
            &response,
            sizeof(response),
            &response_length))
    {
        return false;
    }

    if (status != (uint8_t)DEK_SERVICE_COMMAND_STATUS_OK ||
        response_length != sizeof(response) ||
        response.pin != pin)
    {
        return false;
    }

    *value = response.value;
    return true;
}
