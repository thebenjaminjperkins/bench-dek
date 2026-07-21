#ifndef SERVICE_TYPES_H
#define SERVICE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "module-manager/capability_types.h"

typedef struct dek_service_instance dek_service_instance_t;

typedef struct
{
    uint32_t instance_id;
    service_kind_t kind;
    dek_service_instance_t *instance;
    const void *api;
} service_handle_t;

static inline bool service_handle_is_bound(const service_handle_t *handle)
{
    return handle != NULL &&
           handle->instance_id != 0u &&
           handle->kind != SERVICE_KIND_NONE &&
           handle->instance != NULL;
}

#endif
