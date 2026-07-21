#ifndef SERVICE_TYPES_H
#define SERVICE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    SERVICE_TYPE_NONE = 0,
    SERVICE_TYPE_UART = 1,
    SERVICE_TYPE_GPIO = 2,
} service_type_t;

/*
 * A service binding is module-specific. The API table is shared by the service
 * type, while module identifies where commands are routed and ctx carries any
 * per-binding state such as channel IDs or transport state.
 */
typedef struct {
    const void *module;
    void *ctx;
    const void *api;
} service_t;

static inline bool service_is_bound(const service_t *service)
{
    return service != NULL && service->module != NULL && service->api != NULL;
}

#endif // SERVICE_TYPES_H
