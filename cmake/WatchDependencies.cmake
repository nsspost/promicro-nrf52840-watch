set(WATCH_NOG_C_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../external/NOG_C"
    CACHE PATH "Path to the independent NOG_C repository")

set(WATCH_UI_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../external/tsehosense-universal-ui"
    CACHE PATH "Path to the independent Universal UI repository")

option(WATCH_ENABLE_GUI_STACK
       "Build the generic NOG_C engine for watch GUI integration experiments"
       OFF)

if(WATCH_ENABLE_GUI_STACK)
    if(NOT EXISTS "${WATCH_NOG_C_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "NOG_C not found at '${WATCH_NOG_C_DIR}'. "
            "Restore the external repository before enabling the GUI stack.")
    endif()

    if(NOT EXISTS "${WATCH_UI_DIR}/docs/spec-v0.1/PACKAGE_MANIFEST.json")
        message(FATAL_ERROR
            "Universal UI contracts not found at '${WATCH_UI_DIR}'.")
    endif()

    set(GNO_BUILD_DEMO OFF CACHE BOOL "" FORCE)
    set(GNO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
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

