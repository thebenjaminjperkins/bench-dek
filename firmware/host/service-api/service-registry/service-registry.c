#include "service-registry.h"

#include <string.h>

bool service_registry_init(service_registry_t *registry)
{
    size_t i;

    if (registry == NULL) {
        return false;
    }

    registry->count = 0;

    for (i = 0; i < MAX_SERVICES; ++i) {
        registry->services[i].name = NULL;
        registry->services[i].type = SERVICE_TYPE_NONE;
        registry->services[i].service.module = NULL;
        registry->services[i].service.ctx = NULL;
        registry->services[i].service.api = NULL;
    }

    return true;
}

bool service_registry_register(
    service_registry_t *registry,
    const char *name,
    service_type_t type,
    const void *module,
    void *ctx,
    const void *api)
{
    service_entry_t *entry;

    if (registry == NULL || name == NULL || type == SERVICE_TYPE_NONE || module == NULL || api == NULL) {
        return false;
    }

    if (registry->count >= MAX_SERVICES || service_registry_get_for_module(registry, module, name) != NULL) {
        return false;
    }

    entry = &registry->services[registry->count++];
    entry->name = name;
    entry->type = type;
    entry->service.module = module;
    entry->service.ctx = ctx;
    entry->service.api = api;

    return true;
}

const service_entry_t *service_registry_get(service_registry_t *registry, const char *name)
{
    size_t i;

    if (registry == NULL || name == NULL) {
        return NULL;
    }

    for (i = 0; i < registry->count; ++i) {
        if (registry->services[i].name != NULL && strcmp(registry->services[i].name, name) == 0) {
            return &registry->services[i];
        }
    }

    return NULL;
}

const service_entry_t *service_registry_get_for_module(
    service_registry_t *registry,
    const void *module,
    const char *name)
{
    size_t i;

    if (registry == NULL || module == NULL || name == NULL) {
        return NULL;
    }

    for (i = 0; i < registry->count; ++i) {
        const service_entry_t *entry = &registry->services[i];

        if (entry->service.module == module && entry->name != NULL && strcmp(entry->name, name) == 0) {
            return entry;
        }
    }

    return NULL;
}
