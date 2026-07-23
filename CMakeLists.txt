cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake"
    CACHE FILEPATH "ARM toolchain file")
project(promicro_nrf52840_watch C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(watch_firmware
    src/startup_nrf52840.c
    src/main.c
    src/app/gui_state_actions.c
    src/app/watch_face.c
    src/board/promicro_nrf52840.c
    src/drivers/cst816.c
    src/drivers/gc9a01.c
    src/gui/nog_gc9a01_backend.c
    src/gui/watch_text.c
    src/platform/clock_nrf52840.c
    src/platform/memory.c
    statecharts/GuiSm.c
)

set(WATCH_INITIAL_HOUR 12 CACHE STRING "Initial local hour")
set(WATCH_INITIAL_MINUTE 0 CACHE STRING "Initial local minute")
set(WATCH_INITIAL_SECOND 0 CACHE STRING "Initial local second")
set(WATCH_LCD_SPI_MHZ 8 CACHE STRING "LCD SPI clock in MHz (SPIM0: 1, 2, 4, or 8)")
set_property(CACHE WATCH_LCD_SPI_MHZ PROPERTY STRINGS 1 2 4 8)

if(NOT WATCH_LCD_SPI_MHZ MATCHES "^(1|2|4|8)$")
    message(FATAL_ERROR "WATCH_LCD_SPI_MHZ must be 1, 2, 4, or 8 for SPIM0")
endif()

target_include_directories(watch_firmware PRIVATE include statecharts)

target_compile_definitions(watch_firmware PRIVATE
    NRF52840_XXAA
    WATCH_STATUS_LED_PIN=-1
    WATCH_INITIAL_HOUR=${WATCH_INITIAL_HOUR}
    WATCH_INITIAL_MINUTE=${WATCH_INITIAL_MINUTE}
    WATCH_INITIAL_SECOND=${WATCH_INITIAL_SECOND}
    WATCH_LCD_SPI_MHZ=${WATCH_LCD_SPI_MHZ}
)

target_compile_options(watch_firmware PRIVATE
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

# StateSmith keeps generic helpers in generated code even when a small chart
# does not need all of them. Preserve -Werror for every handwritten source.
set_source_files_properties(statecharts/GuiSm.c PROPERTIES
    COMPILE_OPTIONS "-Wno-unused-function"
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
