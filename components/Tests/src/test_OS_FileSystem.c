/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "TestMacros.h"

#include <camkes.h>

#include <string.h>

void test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs);

static OS_FileSystem_Config_t littleCfg =
{
    .type = OS_FileSystem_Type_LITTLEFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};

static OS_FileSystem_Config_t fatCfg =
{
    .type = OS_FileSystem_Type_FATFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};


// Private Functions -----------------------------------------------------------

static void
test_OS_FileSystem_mount(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_mount(hFs));

    TEST_FINISH();
}

static void
test_OS_FileSystem_unmount(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_unmount(hFs));

    TEST_FINISH();
}

static void
test_OS_FileSystem_format(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_format(hFs));

    TEST_FINISH();
}

static void
test_OS_FileSystem(
    OS_FileSystem_Handle_t hFs)
{
    test_OS_FileSystem_format(hFs);
    test_OS_FileSystem_mount(hFs);

    test_OS_FileSystemFile(hFs);

    test_OS_FileSystem_unmount(hFs);
}

// Public Functions ------------------------------------------------------------

int run()
{
    OS_Error_t err;
    OS_FileSystem_Handle_t hFs;

    // Test for LittleFS
    if ((err = OS_FileSystem_init(&hFs, &littleCfg)) !=  OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed.");
        return -1;
    }
    test_OS_FileSystem(hFs);
    if ((err = OS_FileSystem_free(hFs)) !=  OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed.");
        return -1;
    }

    // Test for FatFS
    if ((err = OS_FileSystem_init(&hFs, &fatCfg)) !=  OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed.");
        return -1;
    }
    test_OS_FileSystem(hFs);
    if ((err = OS_FileSystem_free(hFs)) !=  OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed.");
        return -1;
    }

    Debug_LOG_INFO("All tests successfully completed.");

    return 0;
}