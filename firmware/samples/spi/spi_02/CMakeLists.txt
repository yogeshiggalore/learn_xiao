cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(spi_02)

target_sources(app PRIVATE src/main.c)

set(YMPRINT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ym_print)
target_include_directories(app PRIVATE ${YMPRINT_DIR})
file(GLOB_RECURSE YMPRINT_DIR_SOURCES "${YMPRINT_DIR}/*.c" "${YMPRINT_DIR}/*.cpp")
target_sources(app PRIVATE ${YMPRINT_DIR_SOURCES})

set(W25QXX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/w25qxx)
target_include_directories(app PRIVATE ${W25QXX_DIR})
file(GLOB_RECURSE W25QXX_DIR_SOURCES "${W25QXX_DIR}/*.c" "${W25QXX_DIR}/*.cpp")
target_sources(app PRIVATE ${W25QXX_DIR_SOURCES})
