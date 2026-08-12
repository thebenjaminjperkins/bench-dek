#include "bringup/dev_runtime.h"

#include <stdio.h>

#include "bringup/gpio_vertical_slice_app.h"
#include "tests/module_service_unit_test.h"

static dek_host_bringup_mode_t dek_host_bringup_selected_mode(void)
{
    return DEK_HOST_BRINGUP_MODE_UNIT_TESTS_THEN_GPIO;
}

void dek_host_bringup_run(void)
{
    switch (dek_host_bringup_selected_mode())
    {
        case DEK_HOST_BRINGUP_MODE_UNIT_TESTS:
            printf("[BRINGUP] mode=unit_tests\n");
            module_service_unit_test_run();
            break;

        case DEK_HOST_BRINGUP_MODE_GPIO_VERTICAL_SLICE:
            printf("[BRINGUP] mode=gpio_vertical_slice\n");
            gpio_vertical_slice_app_run();
            break;

        case DEK_HOST_BRINGUP_MODE_UNIT_TESTS_THEN_GPIO:
            printf("[BRINGUP] mode=unit_tests_then_gpio\n");
            module_service_unit_test_run();
            gpio_vertical_slice_app_run();
            break;

        default:
            printf("[BRINGUP] invalid mode; falling back to unit tests\n");
            module_service_unit_test_run();
            break;
    }
}
