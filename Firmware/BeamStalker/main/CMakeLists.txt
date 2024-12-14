idf_component_register(SRCS
                    "BeamStalker-cardputer.cpp"
                    INCLUDE_DIRS "."
)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
