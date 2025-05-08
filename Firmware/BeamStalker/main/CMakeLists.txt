set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE APP_SOURCES
    "${SOURCE_DIR}/**.c" 
    "${SOURCE_DIR}/**.cpp" 
)

idf_component_register(
    SRCS
        "BeamStalker.cpp"
        ${APP_SOURCES}
    INCLUDE_DIRS 
        "${SOURCE_DIR}"
        "${SOURCE_DIR}/firmware/modules"
        "${SOURCE_DIR}/firmware/utils"
)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
