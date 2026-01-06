cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(dfu_01)

string(REPLACE "/" ";" _b "${BOARD}")
list(GET _b 0 BOARD_BASE)
set(PM_YML "${CMAKE_CURRENT_LIST_DIR}/pm/${BOARD_BASE}.yml")

# If you prefer exact filenames like xiao_nrf54l15_nrf54l15_cpuapp.yml:
string(REPLACE "/" "_" BOARD_FLAT "${BOARD}")
set(PM_YML_FLAT "${CMAKE_CURRENT_LIST_DIR}/pm/${BOARD_FLAT}.yml")

if(EXISTS "${PM_YML_FLAT}")
  configure_file(${PM_YML_FLAT} ${CMAKE_BINARY_DIR}/pm_static.yml COPYONLY)
elseif(EXISTS "${PM_YML}")
  configure_file(${PM_YML} ${CMAKE_BINARY_DIR}/pm_static.yml COPYONLY)
endif()

target_sources(app PRIVATE src/main.c)
