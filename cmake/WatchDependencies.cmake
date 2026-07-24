set(WATCH_NOG_C_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../external/NOG_C"
    CACHE PATH "Path to the independent NOG_C repository")

set(WATCH_UI_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../docs/tsehosense-universal-ui-spec-v0.1"
    CACHE PATH "Path to the Universal UI v0.1 contracts")

option(WATCH_ENABLE_GUI_STACK
       "Build the generic NOG_C engine for watch GUI integration experiments"
       ON)

if(WATCH_ENABLE_GUI_STACK)
    if(NOT EXISTS "${WATCH_NOG_C_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "NOG_C not found at '${WATCH_NOG_C_DIR}'. "
            "Restore the external repository before enabling the GUI stack.")
    endif()

    if(NOT EXISTS "${WATCH_UI_DIR}/PACKAGE_MANIFEST.json")
        message(WARNING
            "Universal UI contracts not found at '${WATCH_UI_DIR}'. "
            "The test-data GUI can still build, but its contract source "
            "cannot be audited.")
    endif()

    set(GNO_BUILD_DEMO OFF CACHE BOOL "" FORCE)
    set(GNO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WATCH_MAX_DISPLAY_DIMENSION 240 CACHE STRING
        "Largest pixel dimension among display variants in this build")
    math(EXPR WATCH_GNO_LINE_COORD_LIMIT
         "${WATCH_MAX_DISPLAY_DIMENSION} * 4")
    if(WATCH_GNO_LINE_COORD_LIMIT GREATER 16383)
        message(FATAL_ERROR
            "Compact NOG_C line clipping supports display dimensions up to 4095")
    endif()

    set(GNO_LINE_CLIP_BITS 32 CACHE STRING "" FORCE)
    set(GNO_LINE_COORD_LIMIT
        ${WATCH_GNO_LINE_COORD_LIMIT} CACHE STRING "" FORCE)
    set(GNO_USE_LIBC_MEMORY OFF CACHE BOOL "" FORCE)

    add_subdirectory("${WATCH_NOG_C_DIR}"
                     "${CMAKE_CURRENT_BINARY_DIR}/external/NOG_C"
                     EXCLUDE_FROM_ALL)

    target_compile_options(gno PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -ffunction-sections
        -fdata-sections
        -fno-common
    )

    target_link_libraries(watch_firmware PRIVATE gno)
    target_compile_definitions(watch_firmware PRIVATE WATCH_GUI_STACK_PRESENT=1)
endif()
