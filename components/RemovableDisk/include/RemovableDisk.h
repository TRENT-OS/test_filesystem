/**
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
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