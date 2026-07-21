#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

#include "services/services.h"

#define MAX_SERVICES 10

typedef struct {
    const char *name;
    service_type_t type;
    service_t service;
} service_entry_t;

typedef struct {
    service_entry_t services[MAX_SERVICES];
    size_t count;
} service_registry_t;

bool service_registry_init(service_registry_t *registry);

bool service_registry_register(
    service_registry_t *registry,
    const char *name,
    service_type_t type,
    const void *module,
    void *ctx,
    const void *api);

const service_entry_t *service_registry_get(
    service_registry_t *registry,
    const char *name);

const service_entry_t *service_registry_get_for_module(
    service_registry_t *registry,
    const void *module,
    const char *name);

#endif // SERVICE_REGISTRY_H
