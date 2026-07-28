cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake"
    CACHE FILEPATH "ARM toolchain file")
project(promicro_nrf52840_watch C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WATCH_ENABLE_BLE "Enable the S140 BLE/Tseho Link transport" ON)
set(NRF5_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/external/nRF5_SDK_17.1.0_ddde560"
    CACHE PATH "Path to Nordic nRF5 SDK 17.1.0")

add_executable(watch_firmware
    src/startup_nrf52840.c
    src/main.c
    src/app/ui_demo_model.c
    src/app/watch_ui.c
    src/board/promicro_nrf52840.c
    src/drivers/cst816.c
    src/drivers/gc9a01.c
    src/gui/nog_gc9a01_backend.c
    src/gui/strict_context_icons.c
    src/gui/strict_context_text.c
    src/gui/watch_text.c
    src/platform/clock_nrf52840.c
    src/platform/freestanding_memory.c
    src/protocol/tseho_link.c
)

if(WATCH_ENABLE_BLE)
    if(NOT EXISTS "${NRF5_SDK_ROOT}/components/softdevice/s140/headers/ble.h")
        message(FATAL_ERROR
            "nRF5 SDK 17.1.0 is missing. Run scripts/install-nrf5-sdk.ps1")
    endif()
    target_sources(watch_firmware PRIVATE src/platform/ble_softdevice.c)
    target_include_directories(watch_firmware PRIVATE
        "${NRF5_SDK_ROOT}/components/softdevice/s140/headers"
        "${NRF5_SDK_ROOT}/components/softdevice/s140/headers/nrf52"
        "${NRF5_SDK_ROOT}/modules/nrfx/mdk"
        "${NRF5_SDK_ROOT}/components/toolchain/cmsis/include"
    )
    set_source_files_properties(src/platform/ble_softdevice.c PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-parameter")
    set(WATCH_LINKER_SCRIPT
        "${CMAKE_CURRENT_LIST_DIR}/linker/nrf52840_s140.ld")
else()
    set(WATCH_LINKER_SCRIPT
        "${CMAKE_CURRENT_LIST_DIR}/linker/nrf52840.ld")
endif()

set(WATCH_INITIAL_HOUR 12 CACHE STRING "Initial local hour")
set(WATCH_INITIAL_MINUTE 0 CACHE STRING "Initial local minute")
set(WATCH_INITIAL_SECOND 0 CACHE STRING "Initial local second")
set(WATCH_LCD_SPI_MHZ 8 CACHE STRING "LCD SPI clock in MHz (SPIM0: 1, 2, 4, or 8)")
set_property(CACHE WATCH_LCD_SPI_MHZ PROPERTY STRINGS 1 2 4 8)

if(NOT WATCH_LCD_SPI_MHZ MATCHES "^(1|2|4|8)$")
    message(FATAL_ERROR "WATCH_LCD_SPI_MHZ must be 1, 2, 4, or 8 for SPIM0")
endif()

target_include_directories(watch_firmware PRIVATE include)

target_compile_definitions(watch_firmware PRIVATE
    NRF52840_XXAA
    WATCH_STATUS_LED_PIN=-1
    WATCH_INITIAL_HOUR=${WATCH_INITIAL_HOUR}
    WATCH_INITIAL_MINUTE=${WATCH_INITIAL_MINUTE}
    WATCH_INITIAL_SECOND=${WATCH_INITIAL_SECOND}
    WATCH_LCD_SPI_MHZ=${WATCH_LCD_SPI_MHZ}
    $<$<BOOL:${WATCH_ENABLE_BLE}>:WATCH_ENABLE_BLE>
    $<$<BOOL:${WATCH_ENABLE_BLE}>:S140>
    $<$<BOOL:${WATCH_ENABLE_BLE}>:SOFTDEVICE_PRESENT>
)

set(WATCH_COMMON_COMPILE_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -ffunction-sections
    -fdata-sections
    -fno-common
    -ffreestanding
    -fno-builtin
    -Wall
    -Wextra
    -Werror
    -Og
    -g3
)

target_compile_options(watch_firmware PRIVATE ${WATCH_COMMON_COMPILE_OPTIONS})

add_library(tseho_link_compile_tests OBJECT
    tests/tseho_link_test.c
    src/protocol/tseho_link.c
)
target_include_directories(tseho_link_compile_tests PRIVATE include)
target_compile_options(tseho_link_compile_tests
    PRIVATE ${WATCH_COMMON_COMPILE_OPTIONS})

target_link_options(watch_firmware PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -T${WATCH_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/watch_firmware.map
    -nostdlib
)

set_target_properties(watch_firmware PROPERTIES SUFFIX ".elf")

include(cmake/WatchDependencies.cmake)

add_custom_command(TARGET watch_firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:watch_firmware>
            ${CMAKE_CURRENT_BINARY_DIR}/watch_firmware.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:watch_firmware>
            ${CMAKE_CURRENT_BINARY_DIR}/watch_firmware.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:watch_firmware>
    COMMENT "Creating HEX/BIN and printing image size"
)
