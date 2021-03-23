/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_Crypto.h"

#include "lib_macros/Test.h"
#include "RemovableDisk.h"

#include <camkes.h>

#include <string.h>

void test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type);
void test_OS_FileSystemFile_removal(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type);

//------------------------------------------------------------------------------
static const OS_FileSystem_Format_t littleFsFormat =
{
    .littleFs = {
        .readSize = 4096,
        .writeSize = 4096,
        .blockSize = 4096,
        .blockCycles = 500,
    }
};

static OS_FileSystem_Config_t littleCfg =
{
    .type = OS_FileSystem_Type_LITTLEFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .format = &littleFsFormat,
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


//------------------------------------------------------------------------------
static if_OS_Storage_t storage =
    IF_OS_STORAGE_ASSIGN(
        storage_rpc,
        storage_dp);


// Private Functions -----------------------------------------------------------


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_mount(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    TEST_START("i", expectRemoval, "i", type);

    if (expectRemoval)
    {
        ASSERT_EQ_OS_ERR(OS_ERROR_DEVICE_NOT_PRESENT, OS_FileSystem_mount(hFs));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystem_mount(hFs));
    }

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_unmount(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    TEST_START("i", expectRemoval, "i", type);

    if (expectRemoval)
    {
        ASSERT_EQ_OS_ERR(
            OS_ERROR_DEVICE_NOT_PRESENT,
            OS_FileSystem_unmount(hFs));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystem_unmount(hFs));
    }

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_format(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    TEST_START("i", expectRemoval, "i", type);

    if (expectRemoval)
    {
        ASSERT_EQ_OS_ERR(
            OS_ERROR_DEVICE_NOT_PRESENT,
            OS_FileSystem_format(hFs));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystem_format(hFs));
    }

    TEST_FINISH();
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type)
{
    test_OS_FileSystem_format(hFs, type, false);
    test_OS_FileSystem_mount(hFs, type, false);

    test_OS_FileSystemFile(hFs, type);

    test_OS_FileSystem_unmount(hFs, type, false);
}


//------------------------------------------------------------------------------
static void
test_OS_FileSystem_removal(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type)
{
    DISK_REMOVE;
    test_OS_FileSystem_format(hFs, type, true);
    DISK_ATTACH;

    test_OS_FileSystem_format(hFs, type, false);

    DISK_REMOVE;
    test_OS_FileSystem_mount(hFs, type, true);
    DISK_ATTACH;

    test_OS_FileSystem_format(hFs, type, false);
    test_OS_FileSystem_mount(hFs, type,  false);

    test_OS_FileSystemFile_removal(hFs, type);

    DISK_REMOVE;
    // We don't expect unmount to fail, because it should actually not touch
    // the disk (and thus not perform any I/O which would trigger the error)
    test_OS_FileSystem_unmount(hFs, type, false);
    DISK_ATTACH;
}


//------------------------------------------------------------------------------
// High level Tests
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_cfg(
    OS_FileSystem_Config_t* cfg)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, cfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    test_OS_FileSystem(hFs, cfg->type);
    test_OS_FileSystem_removal(hFs, cfg->type);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %d", ret);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_little_fs(void)
{
    return test_OS_FileSystem_cfg(&littleCfg);
}


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_spiffs(void)
{
    return test_OS_FileSystem_cfg(&spiffsCfg);
}


//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_fat(void)
{
    return test_OS_FileSystem_cfg(&fatCfg);
}

//------------------------------------------------------------------------------
static OS_Error_t
hashStorage(
    OS_Crypto_Handle_t hCrypto,
    uint8_t*           hash,
    size_t             len)
{
    OS_Error_t err;
    OS_CryptoDigest_Handle_t hDigest;
    size_t rd, sz = len;
    off_t bsz, left, hashed;

    /*
     * Here we read the entire storage area and compute the SHA256 hash.
     */

    if ((err = OS_CryptoDigest_init(
                   &hDigest,
                   hCrypto,
                   OS_CryptoDigest_ALG_SHA256)) != OS_SUCCESS)
    {
        return err;
    }

    if ((err = storage.getSize(&left)) != OS_SUCCESS)
    {
        goto out;
    }

    hashed  = 0;
    bsz     = OS_Dataport_getSize(storage.dataport);

    while (left > 0)
    {
        if ((err = storage.read(hashed, bsz, &rd)) != OS_SUCCESS)
        {
            goto out;
        }
        if ((err = OS_CryptoDigest_process(
                       hDigest,
                       OS_Dataport_getBuf(storage.dataport),
                       rd)) != OS_SUCCESS)
        {
            goto out;
        }

        hashed  += rd;
        left    -= rd;
        bsz      = (left < bsz) ? left : bsz;
    }

    err = OS_CryptoDigest_finalize(hDigest, hash, &sz);

out:
    OS_CryptoDigest_free(hDigest);
    return err;
}

//------------------------------------------------------------------------------
/**
 * This macro mounts a fs with one fs format, then computes a hash on it. Then it
 * tries mounting with a different fs format, and again computes a hash. We expect
 * that mounting fails, but the hash remains the same (i.e., mounting does not
 * touch the storage) ...
 */
#define FORMAT_AND_MOUNT(_fs_, _fmt_, _mnt_, _cr_, _h0_, _h1_) \
    { \
        TEST_SUCCESS(OS_FileSystem_init(&_fs_, &_fmt_)); \
        TEST_SUCCESS(OS_FileSystem_format(_fs_)); \
        TEST_SUCCESS(OS_FileSystem_free(_fs_)); \
        TEST_SUCCESS(hashStorage(_cr_, _h0_, sizeof(_h0_))); \
        TEST_SUCCESS(OS_FileSystem_init(&_fs_, &_mnt_));\
        TEST_NOT_FOUND(OS_FileSystem_mount(_fs_)); \
        TEST_SUCCESS(OS_FileSystem_free(_fs_)); \
        TEST_SUCCESS(hashStorage(_cr_, _h1_, sizeof(_h1_))); \
        TEST_TRUE(!memcmp(_h1_, _h0_, sizeof(_h0_))); \
    }

//------------------------------------------------------------------------------
static OS_Error_t
test_OS_FileSystem_mount_fail(void)
{
    static OS_Crypto_Config_t cfgCrypto =
    {
        .mode = OS_Crypto_MODE_LIBRARY,
        .entropy = IF_OS_ENTROPY_ASSIGN(
            entropy_rpc,
            entropy_port),
    };
    OS_FileSystem_Handle_t hFs;
    OS_Crypto_Handle_t hCrypto;
    uint8_t hash0[32], hash1[32];

    TEST_START();

    TEST_SUCCESS(OS_Crypto_init(&hCrypto, &cfgCrypto));

    // Format with FAT, mount with others
    FORMAT_AND_MOUNT(hFs, fatCfg, littleCfg, hCrypto, hash0, hash1);
    FORMAT_AND_MOUNT(hFs, fatCfg, spiffsCfg, hCrypto, hash0, hash1);

    // Format with LittleFS, mount with others
    FORMAT_AND_MOUNT(hFs, littleCfg, fatCfg, hCrypto, hash0, hash1);
    FORMAT_AND_MOUNT(hFs, littleCfg, spiffsCfg, hCrypto, hash0, hash1);

    // Format with SPIFFS, mount with others
    FORMAT_AND_MOUNT(hFs, spiffsCfg, fatCfg, hCrypto, hash0, hash1);
    FORMAT_AND_MOUNT(hFs, spiffsCfg, littleCfg, hCrypto, hash0, hash1);

    TEST_SUCCESS(OS_Crypto_free(hCrypto));

    TEST_FINISH();

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
#define DO_RUN_TEST_SCENARIO(_test_scenario_func_) \
    { \
        OS_Error_t ret = _test_scenario_func_(); \
        if (ret != OS_SUCCESS) \
        { \
            Debug_LOG_ERROR( #_test_scenario_func_ "() FAILED, code %d", ret); \
        } \
        else \
        { \
            Debug_LOG_INFO( #_test_scenario_func_ "() successful"); \
        } \
    }

// Public Functions ------------------------------------------------------------


//------------------------------------------------------------------------------
int run()
{
    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_little_fs );
    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_spiffs );
    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_fat );

    DO_RUN_TEST_SCENARIO( test_OS_FileSystem_mount_fail );

    Debug_LOG_INFO("All test scenarios completed");

    return 0;
}
