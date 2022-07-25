#
# Test Filesystem as a server
#
# Copyright (C) 2020-2022, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

#-------------------------------------------------------------------------------
# use the SDK
find_package("os-sdk" REQUIRED)
os_sdk_set_defaults()
os_sdk_setup(CONFIG_FILE "system_config.h" CONFIG_PROJECT "system_config")


#-------------------------------------------------------------------------------
project(RemovableDisk_client C)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/components/RemovableDisk/include"
)


#-------------------------------------------------------------------------------
project(test_filesystem C)

DeclareCAmkESComponent(
    test_OS_FileSystem
    SOURCES
        components/Tests/src/test_OS_FileSystem.c
        components/Tests/src/test_OS_FileSystemFile.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        lib_macros
        os_crypto
        os_filesystem
        RemovableDisk_client
)

DeclareCAmkESComponent(
    RemovableDisk
    SOURCES
        components/RemovableDisk/src/storage_rpc.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
)

EntropySource_DeclareCAmkESComponent(
    DummyEntropy
)

os_sdk_create_CAmkES_system("main.camkes")
