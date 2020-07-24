/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "TestMacros.h"

#include <string.h>

static const char* fileName = "testfile.txt";
static const uint8_t fileData[] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};
static OS_FileSystemFile_Handle_t hFile;

// Private Functions -----------------------------------------------------------

static void
test_OS_FileSystemFile_open(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_open(hFs, &hFile, fileName,
                                         OS_FileSystem_OpenMode_RDWR,
                                         OS_FileSystem_OpenFlags_CREATE));

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_close(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_close(hFs, hFile));

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_read(
    OS_FileSystem_Handle_t hFs)
{
    uint8_t buf[16];

    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_read(hFs, hFile, 0, sizeof(fileData), buf));
    TEST_TRUE(!memcmp(fileData, buf, sizeof(fileData)));

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_write(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_write(hFs, hFile, 0, sizeof(fileData), fileData));

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_delete(
    OS_FileSystem_Handle_t hFs)
{
    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_delete(hFs, fileName));

    TEST_FINISH();
}

static void
test_OS_FileSystemFile_getSize(
    OS_FileSystem_Handle_t hFs)
{
    off_t size;

    TEST_START();

    TEST_SUCCESS(OS_FileSystemFile_getSize(hFs, fileName, &size));
    TEST_TRUE(size == sizeof(fileData));

    TEST_FINISH();
}

// Public Functions ------------------------------------------------------------

void
test_OS_FileSystemFile(
    OS_FileSystem_Handle_t hFs)
{
    test_OS_FileSystemFile_open(hFs);
    test_OS_FileSystemFile_write(hFs);
    test_OS_FileSystemFile_read(hFs);
    test_OS_FileSystemFile_close(hFs);

    test_OS_FileSystemFile_getSize(hFs);
    test_OS_FileSystemFile_delete(hFs);
}
