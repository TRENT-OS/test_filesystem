/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */


#include "OS_Error.h"

#include "lib_debug/Debug.h"

#include <stdint.h>
#include <string.h>
#include <camkes.h>

#include "system_config.h"

static uint8_t storage[CAMKES_CONST_ATTR(storage_size)] = { 0u };

static int opsCountdown = -1;

// Private Functions -----------------------------------------------------------

static
bool
isValidStorageArea(
    off_t const offset,
    off_t const size)
{
    uintmax_t const end = (uintmax_t)offset + (uintmax_t)size;

    return ((offset >= 0)
            && (size >= 0)
            && (end >= offset)
            && (end <= sizeof(storage)));
}

static
bool
isMediumPresent(
    void)
{
    /*
     * This has three behaviors:
     * 1. If opsCountdown == 0, medium is not present
     * 2. If opsCountdown != 0, medium is present
     * 2.1 Count down to 0 if opsCountdown > 0
     * 2.2 Just leave the opsCountdown value if < 0; this allows to leave the
     *     disk in a permanent "ready" mode.
     */

    if (!opsCountdown) {
        return false;
    }
    if (opsCountdown > 0) {
        opsCountdown = opsCountdown - 1;
    }
    return true;
}

// Public Functions ------------------------------------------------------------

OS_Error_t
NONNULL_ALL
storage_rpc_write(
    off_t   const offset,
    size_t  const size,
    size_t* const written)
{
    if (!isMediumPresent()) {
        return OS_ERROR_DEVICE_NOT_PRESENT;
    }
    if (!isValidStorageArea(offset, size))
    {
        *written = 0U;
        return OS_ERROR_OUT_OF_BOUNDS;
    }

    memcpy(&storage[offset], storage_port, size);
    *written = size;

    return OS_SUCCESS;
}

OS_Error_t
NONNULL_ALL
storage_rpc_read(
    off_t   const offset,
    size_t  const size,
    size_t* const read)
{
    if (!isMediumPresent()) {
        return OS_ERROR_DEVICE_NOT_PRESENT;
    }
    if (!isValidStorageArea(offset, size))
    {
        *read = 0U;
        return OS_ERROR_OUT_OF_BOUNDS;
    }

    memcpy(storage_port, &storage[offset], size);
    *read = size;

    return OS_SUCCESS;
}

OS_Error_t
NONNULL_ALL
storage_rpc_erase(
    off_t  const offset,
    off_t  const size,
    off_t* const erased)
{
    *erased = 0;

    if (!isMediumPresent()) {
        return OS_ERROR_DEVICE_NOT_PRESENT;
    }
    if (!isValidStorageArea(offset, size))
    {
        return OS_ERROR_OUT_OF_BOUNDS;
    }

    memset(&storage[offset], 0xFF, size);
    *erased = size;

    return OS_SUCCESS;
}

OS_Error_t
NONNULL_ALL
storage_rpc_getSize(
    off_t* const size)
{
    if (!isMediumPresent()) {
        return OS_ERROR_DEVICE_NOT_PRESENT;
    }

    *size = sizeof(storage);

    return OS_SUCCESS;
}

OS_Error_t
NONNULL_ALL
storage_rpc_getBlockSize(
    size_t* const blockSize)
{
    *blockSize = 1U;
    return OS_SUCCESS;
}

OS_Error_t
NONNULL_ALL
storage_rpc_getState(
    uint32_t* flags)
{
    if (!isMediumPresent()) {
        return OS_ERROR_DEVICE_NOT_PRESENT;
    }

    *flags = 0U;
    return OS_ERROR_NOT_SUPPORTED;
}

OS_Error_t
disk_rpc_triggerRemoval(
    int ops)
{
    /*
     * We can trigger the OS_ERROR_DEVICE_NOT_PRESENT error by setting the
     * "countdown" value, which is decremented for every storage I/O operation.
     *
     * If we set it to 0, the next I/O op will produces this error, if we set
     * it to X > 0, then the error will occur after X ops. If we set it to X < 0,
     * there will be no error.
     */

    opsCountdown = ops;

    return OS_SUCCESS;
}