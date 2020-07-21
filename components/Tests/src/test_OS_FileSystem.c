/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "libs/os_filesystem/include/OS_FileSystem_int.h"

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


static OS_FileSystem_Config_t spiffsCfg =
{
    .type = OS_FileSystem_Type_SPIFFS,
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

//------------------------------------------------------------------------------
// High level Tests
//------------------------------------------------------------------------------

static void
test_OS_FileSystem_little_fs(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &littleCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %i", ret);
        return;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %i", ret);
        return;
    }
}


static void
test_OS_FileSystem_spiffs(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &spiffsCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %i", ret);
        return;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %i", ret);
        return;
    }
}

static void
test_OS_FileSystem_fat(void)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    if ((ret = OS_FileSystem_init(&hFs, &fatCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %i", ret);
        return;
    }

    test_OS_FileSystem(hFs);

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %i", ret);
        return;
    }
}


static uint8_t work_buf0[256] __attribute__((aligned(256)));
static uint8_t work_buf1[256] __attribute__((aligned(256)));
static uint8_t work_buf2[256] __attribute__((aligned(256),unused));
static uint8_t work_buf3[256] __attribute__((aligned(256),unused));


static OS_Error_t
erase_blocks(size_t start_addr, size_t end_addr, size_t block_sz)
{
    size_t addr;
    size_t bytes_erased;
    OS_Error_t err;
    
    for(addr = start_addr; addr < end_addr; addr += block_sz)
    {
      err = spiffsCfg.storage.erase(addr, block_sz, &bytes_erased);

      if ((err != OS_SUCCESS) || (bytes_erased != block_sz))
      {
	Debug_LOG_ERROR("erase failed %08x, %x %x", addr, block_sz, bytes_erased);
	return OS_ERROR_ABORTED;
      }
    }
    return OS_SUCCESS;

}

static OS_Error_t
read_page(size_t addr, size_t sz, uint8_t *dst)
{
    size_t bytes_read;
    OS_Error_t err;
  
    if ((err = spiffsCfg.storage.read(addr, sz, &bytes_read)) != OS_SUCCESS)
    {
        return err;
    }
    
    if (bytes_read != sz)
    {
      return OS_ERROR_ABORTED;
    }

    memcpy(dst, OS_Dataport_getBuf(spiffsCfg.storage.dataport), bytes_read);
    return OS_SUCCESS;
}


static OS_Error_t
write_page(size_t addr, size_t sz, uint8_t *src)
{
    OS_Error_t err;
    size_t bytes_written;
    
    memcpy(OS_Dataport_getBuf(spiffsCfg.storage.dataport),  src, sz);
    if ((err = spiffsCfg.storage.write(addr, sz, &bytes_written)) != OS_SUCCESS)
    {
        return err;
    }
      
    if (bytes_written != sz)
    {
        return OS_ERROR_ABORTED;
    }
    return OS_SUCCESS;
}

static OS_Error_t
write_validate_page(size_t start_addr, size_t end_addr, size_t page_sz,
		    uint8_t *buf0, uint8_t *buf1)
{
    OS_Error_t err;
    int serr;
    size_t addr;

    for(addr =  0; addr < end_addr; addr += page_sz)
    {
      if((err = write_page(addr, page_sz, buf1)) != OS_SUCCESS)
      {
	return err;
      }
      if((err = read_page(addr, page_sz, buf0)) != OS_SUCCESS)
      {
	return err;
      }
      serr = memcmp(buf0, buf1, page_sz);
      if (serr != 0) {
	return OS_ERROR_ABORTED;
      }
    }
    return OS_SUCCESS;
}

static OS_Error_t
read_validate_page(size_t start_addr, size_t end_addr, size_t page_sz,
		   uint8_t *buf0, uint8_t *buf1)
{
    OS_Error_t err;
    int serr;
    size_t addr;
    
    for(addr = start_addr; addr < end_addr; addr += page_sz)
    {

      if((err = read_page(addr, page_sz, buf0)) != OS_SUCCESS)
      {
  	  return err;
      }

      serr = memcmp(buf0, buf1, page_sz);
      if (serr != 0) {
  	return OS_ERROR_ABORTED;
      }
    }
    return OS_SUCCESS;
}

static void
test_OS_BlockAccess(void)
{
    OS_Error_t err;
    OS_FileSystem_Handle_t hFs;
    size_t sz;
    size_t block_sz = 0x1000;
    size_t page_sz  =  0x100;
    
    if ((err = OS_FileSystem_init(&hFs, &spiffsCfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %i", err);
        return;
    }

    sz = hFs->cfg.size;


//    if (sz > 0x20000) {
//      sz = 0x20000;
//    }
    
    erase_blocks(0, sz, block_sz);
    

    if (page_sz > OS_Dataport_getSize(spiffsCfg.storage.dataport))
    {
        Debug_LOG_ERROR("dataport buffer too small %x %x", OS_Dataport_getSize(spiffsCfg.storage.dataport), page_sz);
	return;
    }

    memset(work_buf1, 0xff, sizeof(work_buf1));

    if ((err = read_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("read_validate_page failed");
        return;
    }
    

    memset(work_buf0, 0xa5, sizeof(work_buf0));
    memset(work_buf1, 0xa5, sizeof(work_buf1));

    if ((err = write_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write_validate_page failed");
	return;
    }


    memset(work_buf0, 0, sizeof(work_buf0));
    memset(work_buf1, 0, sizeof(work_buf1));

    if ((err = write_validate_page(0, sz, page_sz, work_buf0, work_buf1)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write_validate_page failed");
	return;
    }
}
// Public Functions ------------------------------------------------------------



int run()
{
//    test_OS_FileSystem_little_fs();
//    test_OS_FileSystem_fat();
  (void) test_OS_FileSystem_fat;
  (void) test_OS_FileSystem_little_fs;
  
  //  test_OS_FileSystem_spiffs();
  (void) test_OS_FileSystem_spiffs;

  test_OS_BlockAccess();
  
    

    
    Debug_LOG_INFO("All tests successfully completed.");

    return 0;

//    out:
//    Debug_LOG_INFO("Test(s) failed.");
//
//    return OS_ERROR_ABORTED;
}
