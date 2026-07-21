#ifndef SERVICE_API_H
#define SERVICE_API_H

#include <stdbool.h>

#include "service-registry/service-registry.h"

bool service_api_lookup(
    service_registry_t *registry,
    const void *module,
    const char *name,
    service_type_t expected_type,
    service_t *out_service);

#endif // SERVICE_API_H
