cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake"
    CACHE FILEPATH "ARM toolchain file")
project(promicro_nrf52840_watch C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(watch_firmware
    src/startup_nrf52840.c
    src/main.c
)

target_compile_definitions(watch_firmware PRIVATE
    NRF52840_XXAA
    WATCH_STATUS_LED_PIN=-1
)

target_compile_options(watch_firmware PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Werror
    -Og
    -g3
)

target_link_options(watch_firmware PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -T${CMAKE_CURRENT_LIST_DIR}/linker/nrf52840.ld
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
