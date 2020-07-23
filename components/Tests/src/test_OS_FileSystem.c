/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "os_filesystem/include/OS_FileSystem_int.h"
#undef MIN
#undef MAX


#include "TestMacros.h"

#include <camkes.h>
static OS_Dataport_t port_storage  = OS_DATAPORT_ASSIGN(storage_dp);

#include <string.h>

void test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs);


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t littleCfg =
{
    .type = OS_FileSystem_Type_LITTLEFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc,
        storage_dp),
};


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t fatCfg =
{
    .type = OS_FileSystem_Type_FATFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc,
        storage_dp),
};


//------------------------------------------------------------------------------
static OS_FileSystem_Config_t spiffsCfg =
{
    .type = OS_FileSystem_Type_SPIFFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
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
#define BLOCK_SZ    4096
#define PAGE_SZ      256


//------------------------------------------------------------------------------
static OS_Error_t
__attribute__((unused))
read_validate(
    size_t addr,
    size_t sz,
    uint8_t *buf,
    const uint8_t *buf_ref)
{
    // Debug_LOG_INFO("read_validate addr=0x%x, len=0x%0x", addr, sz);

    size_t bytes_read = 0;
    OS_Error_t ret = storage_rpc_read(addr, sz, &bytes_read);
    if ((OS_SUCCESS != ret) || (bytes_read != sz))
    {
        Debug_LOG_ERROR(
            "storage_rpc_read failed, addr=0x%x, sz=0x%x, read=0x%x, code %d",
            addr, sz, bytes_read, ret);
        return OS_ERROR_GENERIC;
    }

    const void* data = OS_Dataport_getBuf(port_storage);
    int serr = memcmp(data, buf_ref, sz);
    if (0 != serr)
    {
        Debug_LOG_ERROR("memcmp failed");
        Debug_DUMP_INFO(data, sz);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------
static OS_Error_t
__attribute__((unused))
test_flash_block(
    const size_t addr,
    void* buf,
    const void* buf_ref_empty,
    const void* buf_ref_pattern)
{
    OS_Error_t ret;

    size_t bytes_erased = 0;
    ret = storage_rpc_erase(addr, BLOCK_SZ, &bytes_erased);

    if ((ret != OS_SUCCESS) || (bytes_erased != BLOCK_SZ))
    {
        Debug_LOG_ERROR("erase failed len_ret=%x, code %d", bytes_erased, ret);
        return OS_ERROR_ABORTED;
    }

    ret = read_validate(addr, BLOCK_SZ, buf, buf_ref_empty);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("erase 0xFF validation, code %d", ret);
        return OS_ERROR_ABORTED;
    }

    for (unsigned int i = 0; i<(BLOCK_SZ/PAGE_SZ); i++)
    {
        memcpy(OS_Dataport_getBuf(port_storage), buf_ref_pattern, PAGE_SZ);
        size_t write_addr = addr + i*PAGE_SZ;
        size_t bytes_written = 0;
        ret = storage_rpc_write(write_addr, PAGE_SZ, &bytes_written);
        if ((OS_SUCCESS != ret) || (bytes_written != PAGE_SZ))
        {
            Debug_LOG_ERROR(
                "storage_rpc_write failed, addr=0x%x, sz=0x%x, read=0x%x, code %d",
                write_addr, PAGE_SZ, bytes_written, ret);
        }
    }

    ret = read_validate(addr, BLOCK_SZ, buf, buf_ref_pattern);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("pattern validation failed, code %d", ret);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------
static void
__attribute__((unused))
test_OS_BlockAccess(void)
{

    static uint8_t buf[BLOCK_SZ];

    static uint8_t buf_ref_empty[BLOCK_SZ];
    memset(buf_ref_empty, 0xff, sizeof(buf_ref_empty));

    static uint8_t buf_ref_pattern_block_0[BLOCK_SZ];
    memset(buf_ref_pattern_block_0, 0x5a, sizeof(buf_ref_pattern_block_0));

    static uint8_t buf_ref_pattern[BLOCK_SZ];
    memset(buf_ref_pattern, 0xa5, sizeof(buf_ref_pattern));

    OS_Error_t ret;
    size_t sz;
    if ((ret = storage_rpc_getSize(&sz)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("storage_rpc_getSize() failed, code %d", ret);
        return;
    }


    Debug_LOG_INFO("test addr 0x0");
    test_flash_block(0, buf, buf_ref_empty, buf_ref_pattern_block_0);

    for (unsigned int i = 0; i < 14; i++)
    {
        size_t addr = BLOCK_SZ << i;
        Debug_LOG_INFO("test 2^n addr: 0x0%x", addr);
        ret = test_flash_block(
                addr,
                buf,
                buf_ref_empty,
                buf_ref_pattern);
        if (OS_SUCCESS != ret)
        {
            Debug_LOG_ERROR(
                "test_flash_block 0 failed at addr 0x%x, code %d",
                addr, ret);
            break;
        }

        ret = read_validate(0, BLOCK_SZ, buf, buf_ref_pattern_block_0);
        if (OS_SUCCESS != ret)
        {
            Debug_LOG_ERROR(
                "read_validate block 0 pattern failed at addr 0x%x, code %d",
                addr, ret);
            break;
        }

    }

    const size_t start_addr = 0x5FC000;
    const size_t end_addr = 0x800000;
    for(size_t addr = start_addr; addr < end_addr; addr += BLOCK_SZ)
    {
        Debug_LOG_INFO("test all blocks: 0x0%x", addr);
        test_flash_block(addr, buf, buf_ref_empty, buf_ref_pattern);
    }


    // if (page_sz > OS_Dataport_getSize(port_storage))
    // {
    //     Debug_LOG_ERROR("dataport buffer too small %x %x", OS_Dataport_getSize(port_storage), page_sz);
    //     return;
    // }
    //
    // memset(work_buf1, 0xff, sizeof(work_buf1));
    //
    // if ((err = read_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    // {
    //     Debug_LOG_ERROR("read_validate_page failed");
    //     return;
    // }
    //
    // memset(work_buf0, 0xa5, sizeof(work_buf0));
    // memset(work_buf1, 0xa5, sizeof(work_buf1));
    //
    // if ((err = write_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    // {
    //     Debug_LOG_ERROR("write_validate_page failed");
    //     return;
    // }
    //
    //
    // memset(work_buf0, 0, sizeof(work_buf0));
    // memset(work_buf1, 0, sizeof(work_buf1));
    //
    // if ((err = write_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    // {
    //     Debug_LOG_ERROR("write_validate_page failed");
    //     return;
    // }
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
    Debug_LOG_INFO("calling test_OS_BlockAccess() ...");
    test_OS_BlockAccess();
    Debug_LOG_INFO("test_OS_BlockAccess() done");

    (void)test_OS_FileSystem_little_fs;
    (void)test_OS_FileSystem_spiffs;
    (void)test_OS_FileSystem_fat;
    Debug_LOG_INFO("All test scenarios completed");

    return 0;
}
