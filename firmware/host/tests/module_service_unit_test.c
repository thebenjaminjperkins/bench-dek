#include "module_service_unit_test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "capabilities/dek_capability_ids.h"
#include "message-types/dek_descriptor.h"
#include "module-manager/module_registry.h"
#include "module-manager/service_instance.h"
#include "service-api/service_api.h"

static void print_test_banner(const char *test_name)
{
    printf("\n[TEST] %s\n", test_name);
}

static void print_step(const char *test_name, const char *step_name)
{
    printf("  [%s] %s\n", test_name, step_name);
}

static bool require_true(
    const char *test_name,
    const char *step_name,
    bool condition,
    const char *failure_message)
{
    print_step(test_name, step_name);

    if (!condition)
    {
        printf("    FAIL: %s\n", failure_message);
        return false;
    }

    printf("    OK\n");
    return true;
}

static bool stub_open(
    dek_capability_provider_t *provider,
    const void *config,
    size_t config_length,
    dek_service_instance_t *instance)
{
    (void)provider;
    (void)config;
    (void)config_length;
    (void)instance;
    return true;
}

static bool stub_close(
    dek_capability_provider_t *provider,
    dek_service_instance_t *instance)
{
    (void)provider;
    (void)instance;
    return true;
}

static bool module_service_unit_test_run_impl(void)
{
    static const char *test_name = "module_service_unit_test";
    module_registry_t registry;
    service_instance_registry_t instances;
    service_api_t api;
    dek_descriptor_payload_t descriptor;
    dek_module_record_t *module_record = NULL;
    dek_capability_provider_t *provider = NULL;
    service_handle_t handle = { 0 };
    const dek_service_instance_t *instance = NULL;
    uint32_t instance_id = 0u;
    uint8_t module_id[DEK_MODULE_ID_SIZE] = { 0x10u, 0x20u, 0x30u, 0x40u,
                                               0x50u, 0x60u, 0x70u, 0x80u,
                                               0x90u, 0xA0u, 0xB0u, 0xC0u,
                                               0xD0u, 0xE0u, 0xF0u, 0x01u };
    uint8_t invalid_module_id[DEK_MODULE_ID_SIZE] = { 0x01u, 0x02u, 0x03u, 0x04u,
                                                      0x05u, 0x06u, 0x07u, 0x08u,
                                                      0x09u, 0x0Au, 0x0Bu, 0x0Cu,
                                                      0x0Du, 0x0Eu, 0x0Fu, 0x10u };
    const char *capability_id = DEK_CAPABILITY_ID_UART_STREAM;
    uint16_t capability_version = DEK_CAPABILITY_VERSION_UART_STREAM;
    const dek_capability_provider_api_t provider_api = {
        .open = stub_open,
        .close = stub_close,
    };
    const void *typed_api = (const void *)0x12345678u;
    capability_request_t request = {
        .capability_id = capability_id,
        .capability_version = capability_version,
        .preferred_module_id = module_id,
        .config = NULL,
        .config_length = 0u,
        .lease_owner = 0xAA55AA55u,
    };

    print_test_banner(test_name);

    if (!require_true(
            test_name,
            "initialize registry and service API state",
            module_registry_init(&registry) &&
                service_instance_registry_init(&instances) &&
                service_api_init(&api, &registry, &instances),
            "module registry, service instance registry, and service API should initialize"))
    {
        return false;
    }

    print_step(test_name, "prepare descriptor payload");
    dek_descriptor_payload_init(&descriptor);
    memcpy(descriptor.module_id, module_id, sizeof(module_id));
    descriptor.max_payload_size = 256u;
    descriptor.supported_protocol_version = 1u;

    module_record = module_registry_register_module(&registry, &descriptor);
    if (!require_true(
            test_name,
            "register module record",
            module_record != NULL,
            "module should register successfully"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "reject duplicate module id registration",
            module_registry_register_module(&registry, &descriptor) == NULL,
            "duplicate module ids should be rejected"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "register capability provider",
            module_registry_register_provider(
                &registry,
                module_record,
                capability_id,
                capability_version,
                DEK_RESOURCE_POLICY_SHARED,
                SERVICE_KIND_UART_STREAM,
                NULL,
                &provider_api,
                typed_api),
            "provider should register successfully"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "reject duplicate provider registration",
            !module_registry_register_provider(
                &registry,
                module_record,
                capability_id,
                capability_version,
                DEK_RESOURCE_POLICY_SHARED,
                SERVICE_KIND_UART_STREAM,
                NULL,
                &provider_api,
                typed_api),
            "duplicate provider registration should fail"))
    {
        return false;
    }

    provider = module_registry_find_provider(
        &registry,
        capability_id,
        capability_version,
        module_id);
    if (!require_true(
            test_name,
            "find provider for preferred module id",
            provider != NULL,
            "provider lookup should return the registered provider"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "fail lookup for mismatched preferred module id",
            module_registry_find_provider(
                &registry,
                capability_id,
                capability_version,
                invalid_module_id) == NULL,
            "provider lookup should respect preferred module id"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "open capability service instance",
            service_api_open(&api, &request, &handle),
            "service_api_open should create a handle"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "report handle as bound after open",
            service_handle_is_bound(&handle),
            "opened service handle should be bound"))
    {
        return false;
    }

    instance_id = handle.instance_id;

    if (!require_true(
            test_name,
            "resolve service instance from handle",
            service_api_get_instance(&handle, &instance),
            "service_api_get_instance should succeed"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "verify service instance fields",
            instance != NULL &&
                instance->provider == provider &&
                instance->kind == SERVICE_KIND_UART_STREAM &&
                instance->lease_owner == request.lease_owner &&
                instance->typed_api == typed_api &&
                instance->state == SERVICE_INSTANCE_STATE_OPEN,
            "opened instance should preserve provider, kind, owner, api, and state"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "close service handle",
            service_api_close(&api, &handle),
            "service_api_close should succeed"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "verify handle cleared after close",
            !service_handle_is_bound(&handle),
            "closed handle should no longer be bound"))
    {
        return false;
    }

    if (!require_true(
            test_name,
            "verify instance registry no longer exposes closed instance",
            service_instance_registry_find_by_id(&instances, instance_id) == NULL,
            "closed instance should be released from the registry"))
    {
        return false;
    }

    printf("  [%s] PASS\n", test_name);
    return true;
}

void module_service_unit_test_run(void)
{
    (void)module_service_unit_test_run_impl();
}
