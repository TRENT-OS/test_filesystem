/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "LibMacros/Test.h"
#include "RemovableDisk.h"

#include <string.h>

static const char* fileName = "testfile.txt";
static const uint8_t fileData[] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};
static const off_t fileSize = sizeof(fileData) * 1024;
static OS_FileSystemFile_Handle_t hFile;

// Private Functions -----------------------------------------------------------

static void
test_OS_FileSystemFile_open(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    TEST_START(expectRemoval, type);

    if (expectRemoval)
    {
        TEST_NOT_PRESENT(OS_FileSystemFile_open(hFs, &hFile, fileName,
                                                OS_FileSystem_OpenMode_RDWR,
                                                OS_FileSystem_OpenFlags_CREATE));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystemFile_open(hFs, &hFile, fileName,
                                            OS_FileSystem_OpenMode_RDWR,
                                            OS_FileSystem_OpenFlags_CREATE));
    }

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_close(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    OS_Error_t err;

    TEST_START(expectRemoval, type);

    err = OS_FileSystemFile_close(hFs, hFile);
    if (expectRemoval)
    {
        // So there are FS which actually don't touch the disk during close,
        // thus we would not get the error from the storage layer; others do
        // flush their internal buffers.
        TEST_TRUE(err == OS_SUCCESS || err == OS_ERROR_DEVICE_NOT_PRESENT);
    }
    else
    {
        TEST_SUCCESS(err);
    }

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_read(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    uint8_t buf[sizeof(fileData)];
    off_t to_read, read;
    OS_Error_t err;

    TEST_START(expectRemoval, type);

    to_read = fileSize;
    read    = 0;
    while (to_read > 0)
    {
        err = OS_FileSystemFile_read(hFs, hFile, read, sizeof(buf), buf);
        if (!expectRemoval)
        {
            TEST_SUCCESS(err);
            TEST_TRUE(!memcmp(fileData, buf, sizeof(buf)));
        }
        else
        {
            // We get the NOT_PRESENT immediately, so there is no need to keep
            // trying further..
            TEST_NOT_PRESENT(err);
            break;
        }

        read    += sizeof(buf);
        to_read -= sizeof(buf);
    }

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_write(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    off_t to_write, written;
    OS_Error_t err;

    TEST_START(expectRemoval, type);

    to_write = fileSize;
    written  = 0;
    while (to_write > 0)
    {
        err = OS_FileSystemFile_write(hFs, hFile, written, sizeof(fileData), fileData);
        if (!expectRemoval)
        {
            TEST_SUCCESS(err);
        }
        else
        {
            // Even if the device is removed before we start writing, due to the
            // fact that the FS will internally buffer writes, we may actually
            // not see an error for a while.
            TEST_TRUE(err == OS_SUCCESS || err == OS_ERROR_DEVICE_NOT_PRESENT);
            // Device won't come back, so we might as well stop trying..
            if (err == OS_ERROR_DEVICE_NOT_PRESENT)
            {
                break;
            }
        }

        written  += sizeof(fileData);
        to_write -= sizeof(fileData);
    }

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_delete(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    TEST_START(expectRemoval, type);

    if (expectRemoval)
    {
        TEST_NOT_PRESENT(OS_FileSystemFile_delete(hFs, fileName));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystemFile_delete(hFs, fileName));
    }

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_getSize(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type,
    bool expectRemoval)
{
    off_t size;

    TEST_START(expectRemoval, type);

    if (expectRemoval)
    {
        TEST_NOT_PRESENT(OS_FileSystemFile_getSize(hFs, fileName, &size));
    }
    else
    {
        TEST_SUCCESS(OS_FileSystemFile_getSize(hFs, fileName, &size));
        TEST_TRUE(size == fileSize);
    }

    TEST_FINISH();
}

// Public Functions ------------------------------------------------------------

void
test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type)
{
    test_OS_FileSystemFile_open(hFs, type, false);
    test_OS_FileSystemFile_write(hFs, type, false);
    test_OS_FileSystemFile_read(hFs, type, false);
    test_OS_FileSystemFile_close(hFs, type, false);

    test_OS_FileSystemFile_getSize(hFs, type, false);
    test_OS_FileSystemFile_delete(hFs, type, false);
}

void
test_OS_FileSystemFile_removal(
    OS_FileSystem_Handle_t hFs,
    OS_FileSystem_Type_t type)
{
    DISK_REMOVE;
    test_OS_FileSystemFile_open(hFs, type, true);
    DISK_ATTACH;

    // Now actually open the file
    test_OS_FileSystemFile_open(hFs, type, false);

    DISK_REMOVE;
    test_OS_FileSystemFile_write(hFs, type, true);
    DISK_ATTACH;

    // Writing above should fail at some point, so now we do write something
    // such that the following read has something to fail on..
    test_OS_FileSystemFile_close(hFs, type, false);
    test_OS_FileSystemFile_open(hFs, type, false);
    test_OS_FileSystemFile_write(hFs, type, false);

    DISK_REMOVE;
    test_OS_FileSystemFile_read(hFs, type, true);
    test_OS_FileSystemFile_close(hFs, type, true);
    test_OS_FileSystemFile_delete(hFs, type, true);

    // This is a trick; we first delete the file and then ask for its size, thus
    // effectively forcing the fs layer to check if the file may actually be on
    // storage.. thus triggering the expected fault.
    test_OS_FileSystemFile_getSize(hFs, type, true);
    DISK_ATTACH;
}