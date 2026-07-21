#ifndef SERVICE_INSTANCE_H
#define SERVICE_INSTANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "module_registry.h"

#define MAX_SERVICE_INSTANCES 8u
#define SERVICE_INSTANCE_CONFIG_MAX 64u

typedef enum
{
    SERVICE_INSTANCE_STATE_FREE = 0,
    SERVICE_INSTANCE_STATE_ALLOCATED = 1,
    SERVICE_INSTANCE_STATE_OPEN = 2,
    SERVICE_INSTANCE_STATE_ACTIVE = 3,
    SERVICE_INSTANCE_STATE_FAULTED = 4
} service_instance_state_t;

struct dek_service_instance
{
    bool occupied;
    uint32_t instance_id;
    service_kind_t kind;
    char capability_id[DEK_CAPABILITY_ID_MAX_LENGTH + 1u];
    uint16_t capability_version;
    dek_capability_provider_t *provider;
    const void *typed_api;
    uint16_t channel_id;
    service_instance_state_t state;
    uint32_t lease_owner;
    uint16_t config_length;
    uint8_t config[SERVICE_INSTANCE_CONFIG_MAX];
};

typedef struct
{
    dek_service_instance_t instances[MAX_SERVICE_INSTANCES];
    uint32_t next_instance_id;
} service_instance_registry_t;

bool service_instance_registry_init(service_instance_registry_t *registry);

dek_service_instance_t *service_instance_registry_allocate(
    service_instance_registry_t *registry,
    dek_capability_provider_t *provider,
    const char *capability_id,
    uint16_t capability_version,
    const void *config,
    size_t config_length,
    uint32_t lease_owner);

dek_service_instance_t *service_instance_registry_find_by_id(
    service_instance_registry_t *registry,
    uint32_t instance_id);

dek_service_instance_t *service_instance_registry_find_by_provider(
    service_instance_registry_t *registry,
    const dek_capability_provider_t *provider);

bool service_instance_registry_release(
    service_instance_registry_t *registry,
    dek_service_instance_t *instance);

#endif
