#include "service-gpio.h"

static const gpio_service_api_t *gpio_service_get_api(const service_handle_t *handle)
{
    if (!service_handle_is_bound(handle) ||
        handle->kind != SERVICE_KIND_GPIO_DIGITAL ||
        handle->api == NULL)
    {
        return NULL;
    }

    return (const gpio_service_api_t *)handle->api;
}

bool gpio_service_set_mode(
    const service_handle_t *handle,
    uint8_t pin,
    dek_gpio_digital_pin_mode_t mode)
{
    const gpio_service_api_t *api = gpio_service_get_api(handle);

    if (api == NULL ||
        api->set_mode == NULL ||
        !dek_gpio_digital_pin_mode_is_valid((uint8_t)mode))
    {
        return false;
    }

    return api->set_mode(handle->instance, pin, mode);
}

bool gpio_service_write(
    const service_handle_t *handle,
    uint8_t pin,
    uint8_t value)
{
    const gpio_service_api_t *api = gpio_service_get_api(handle);

    if (api == NULL ||
        api->write == NULL ||
        !dek_gpio_digital_level_is_valid(value))
    {
        return false;
    }

    return api->write(handle->instance, pin, value);
}

bool gpio_service_read(
    const service_handle_t *handle,
    uint8_t pin,
    uint8_t *value)
{
    const gpio_service_api_t *api = gpio_service_get_api(handle);

    if (api == NULL || api->read == NULL || value == NULL)
    {
        return false;
    }

    return api->read(handle->instance, pin, value);
}
