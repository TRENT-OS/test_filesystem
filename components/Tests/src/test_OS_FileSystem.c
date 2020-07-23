/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "TestMacros.h"

#include <camkes.h>

#include <string.h>

void test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs);


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t littleCfg =
{
    .type = OS_FileSystem_Type_LITTLEFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t fatCfg =
{
    .type = OS_FileSystem_Type_FATFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t spiffsCfg =
{
    .type = OS_FileSystem_Type_SPIFFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};


// Private Functions -----------------------------------------------------------


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_mount(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_mount(hFs));

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_unmount(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_unmount(hFs));

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_format(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystem_format(hFs));

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem(
    OS_FileSystem_Handle_t hFs)
{
    test_OS_FileSystem_format(hFs);
    test_OS_FileSystem_mount(hFs);

    test_OS_FileSystemFile(hFs);

    test_OS_FileSystem_unmount(hFs);
}

//------------------------------------------------------------------------------
// High level Tests
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_little_fs(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &littleCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_spiffs(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &spiffsCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %i", ret);
        return OS_ERROR_GENERIC;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %i", ret);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_fat(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &fatCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------
#define DO_RUN_TEST_SCENARIO(_test_scnenario_func_) \
    { \
        OS_Error_t ret = _test_scnenario_func_(); \
        if (ret != OS_SUCCESS) \
        { \
            Debug_LOG_ERROR( #_test_scnenario_func_ "() FAILED, code %d", ret); \
        } \
        else \
        { \
            Debug_LOG_INFO( #_test_scnenario_func_ "() successful"); \
        } \
    }

// Public Functions ------------------------------------------------------------


//------------------------------------------------------------------------------
int run()
{
    // Little FS and SPIFFS will take care of erase operations, so the flash
    // driver does not have to be smart here, it can blindly execute all
    // operations
    adapter_rpc_set_mode(FLASH_ADAPTER_MODE_TRANSPARENT);

    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_little_fs );
    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_spiffs );

    // FAT does not know the concept of flash storage, write overwriting data
    // required an erase operation first. Switch the flash driver into a mode
    // where it does the erasing automatically. Note that this does not give
    // any guarantees how smart the driver implements this actually, the most
    // simple strategy of erasing whole block(s) first and then update them
    // with the changes wears out the flash faster.
    adapter_rpc_set_mode(FLASH_ADAPTER_MODE_ERASE_BEFORE_WRITE);

    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_fat );

    Debug_LOG_INFO("All test scenarios completed");

    return 0;
}
