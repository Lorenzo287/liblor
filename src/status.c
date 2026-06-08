// SPDX-License-Identifier: MIT

#include "lor/status.h"

const char *lor_status_name(LorStatus status) {
    switch (status) {
    case LOR_STATUS_OK:
        return "ok";
    case LOR_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case LOR_STATUS_OUT_OF_MEMORY:
        return "out of memory";
    case LOR_STATUS_OVERFLOW:
        return "overflow";
    case LOR_STATUS_SYSTEM_ERROR:
        return "system error";
    case LOR_STATUS_TIMED_OUT:
        return "timed out";
    case LOR_STATUS_CLOSED:
        return "closed";
    }

    return "unknown";
}
