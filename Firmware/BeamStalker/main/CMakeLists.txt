set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE APP_SOURCES
    "${SOURCE_DIR}/firmware/*.c" 
    "${SOURCE_DIR}/firmware/*.cpp" 
    "${SOURCE_DIR}/firmware/apps/*.c" 
    "${SOURCE_DIR}/firmware/apps/*.cpp" 
    "${SOURCE_DIR}/firmware/includes/*.c"
    "${SOURCE_DIR}/firmware/includes/*.cpp"
)

idf_component_register(
    SRCS
        "BeamStalker.cpp"
        ${APP_SOURCES}
    INCLUDE_DIRS 
        "${SOURCE_DIR}"
        "${SOURCE_DIR}/firmware/apps"
        "${SOURCE_DIR}/firmware/includes"
)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
