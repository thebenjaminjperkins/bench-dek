#ifndef SERVICE_GPIO_H
#define SERVICE_GPIO_H

#include <stddef.h>
#include <stdint.h>

#include "services/services.h"

typedef enum {
    GPIO_PIN_MODE_INPUT = 0,
    GPIO_PIN_MODE_OUTPUT = 1,
} gpio_pin_mode_t;

typedef struct {
    bool (*set_mode)(void *ctx, uint8_t pin, gpio_pin_mode_t mode);
    bool (*write)(void *ctx, uint8_t pin, uint8_t value);
    bool (*read)(void *ctx, uint8_t pin, uint8_t *value);
} gpio_service_api_t;

typedef gpio_service_api_t gpio_service_t;

bool gpio_service_set_mode(const service_t *service, uint8_t pin, gpio_pin_mode_t mode);
bool gpio_service_write(const service_t *service, uint8_t pin, uint8_t value);
bool gpio_service_read(const service_t *service, uint8_t pin, uint8_t *value);

#endif // SERVICE_GPIO_H
