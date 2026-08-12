#ifndef DEK_HOST_BRINGUP_DEV_RUNTIME_H
#define DEK_HOST_BRINGUP_DEV_RUNTIME_H

typedef enum
{
    DEK_HOST_BRINGUP_MODE_UNIT_TESTS = 0,
    DEK_HOST_BRINGUP_MODE_GPIO_VERTICAL_SLICE = 1,
    DEK_HOST_BRINGUP_MODE_UNIT_TESTS_THEN_GPIO = 2
} dek_host_bringup_mode_t;

void dek_host_bringup_run(void);

#endif
