#include "dek_hello.h"
#include <string.h>

void dek_hello_payload_init(dek_hello_payload_t *payload)
{
    if (payload == NULL)
    {
        return;
    }

    payload->min_protocol_version = 0;
    payload->max_protocol_version = 0;
    payload->host_flags = 0;
}