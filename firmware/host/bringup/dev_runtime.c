#include "bringup/dev_runtime.h"

#include "tests/module_service_unit_test.h"

void dek_host_bringup_run(void)
{
    module_service_unit_test_run();
}
