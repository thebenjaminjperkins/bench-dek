#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_main.h"
#include "transport/dek_transport.h"

void dek_host_app_main(void)
{
    if (transport_hello_self_test())
    {
        printf("PASS\n");
    }
    else
    {
        printf("FAIL\n");
    }
}