/**
 * Copyright (C) 2019-2020, Hensoldt Cyber GmbH
 */

#pragma once

#include <camkes.h>

/**
 * Use additional RPC endpoint to switch on "storage removal"; the param we use
 * here means:
 *  0: pretend medium was removed on next storage I/O operation
 * -1: do not pretend medium was removed
 */
#define DISK_REMOVE disk_rpc_triggerRemoval( 0)
#define DISK_ATTACH disk_rpc_triggerRemoval(-1)