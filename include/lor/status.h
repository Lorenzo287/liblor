// SPDX-License-Identifier: MIT

#ifndef LOR_STATUS_H
#define LOR_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LorStatus {
    LOR_STATUS_OK = 0,
    LOR_STATUS_INVALID_ARGUMENT,
    LOR_STATUS_OUT_OF_MEMORY,
    LOR_STATUS_OVERFLOW
} LorStatus;

// Returns a stable lowercase name for `status`, or "unknown".
const char *lor_status_name(LorStatus status);

#ifdef __cplusplus
}
#endif

#endif
