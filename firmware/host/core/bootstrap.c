#include "bootstrap.h"

#include "applications/uart_smoke_app.h"

void dek_bootstrap(void)
{
    uart_smoke_app_run();
}
