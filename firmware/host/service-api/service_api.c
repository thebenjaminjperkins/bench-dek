#include "service_api.h"

bool service_api_lookup(
    service_registry_t *registry,
    const void *module,
    const char *name,
    service_type_t expected_type,
    service_t *out_service)
{
    const service_entry_t *entry;

    if (out_service == NULL) {
        return false;
    }

    entry = service_registry_get_for_module(registry, module, name);
    if (entry == NULL || entry->type != expected_type) {
        return false;
    }

    *out_service = entry->service;
    return true;
}
