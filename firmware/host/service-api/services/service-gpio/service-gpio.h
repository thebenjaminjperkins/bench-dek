#ifndef SERVICE_GPIO_H
#define SERVICE_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "capabilities/dek_gpio_digital.h"
#include "service-api/services/services.h"

typedef struct
{
    bool (*set_mode)(
        dek_service_instance_t *instance,
        uint8_t pin,
        dek_gpio_digital_pin_mode_t mode);
    bool (*write)(
        dek_service_instance_t *instance,
        uint8_t pin,
        uint8_t value);
    bool (*read)(
        dek_service_instance_t *instance,
        uint8_t pin,
        uint8_t *value);
} gpio_service_api_t;

bool gpio_service_set_mode(
    const service_handle_t *handle,
    uint8_t pin,
    dek_gpio_digital_pin_mode_t mode);

bool gpio_service_write(
    const service_handle_t *handle,
    uint8_t pin,
    uint8_t value);

bool gpio_service_read(
    const service_handle_t *handle,
    uint8_t pin,
    uint8_t *value);

#endif
