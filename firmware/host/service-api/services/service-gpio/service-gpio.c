#include "service-gpio.h"

static const gpio_service_api_t *gpio_service_get_api(const service_t *service)
{
    if (!service_is_bound(service)) {
        return NULL;
    }

    return (const gpio_service_api_t *)service->api;
}

bool gpio_service_set_mode(const service_t *service, uint8_t pin, gpio_pin_mode_t mode)
{
    const gpio_service_api_t *api = gpio_service_get_api(service);

    if (api == NULL || api->set_mode == NULL) {
        return false;
    }

    return api->set_mode(service->ctx, pin, mode);
}

bool gpio_service_write(const service_t *service, uint8_t pin, uint8_t value)
{
    const gpio_service_api_t *api = gpio_service_get_api(service);

    if (api == NULL || api->write == NULL) {
        return false;
    }

    return api->write(service->ctx, pin, value);
}

bool gpio_service_read(const service_t *service, uint8_t pin, uint8_t *value)
{
    const gpio_service_api_t *api = gpio_service_get_api(service);

    if (value == NULL || api == NULL || api->read == NULL) {
        return false;
    }

    return api->read(service->ctx, pin, value);
}
