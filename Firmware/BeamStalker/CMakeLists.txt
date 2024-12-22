# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(OUTPUT_NAME "BeamStalker")

file(READ "main/firmware/helper.h" HELPER_CONTENTS)
string(REGEX MATCH "#define VERSION \"([^\"]+)\"" VERSION_MATCH "${HELPER_CONTENTS}")

if(VERSION_MATCH)
    string(REGEX REPLACE "#define VERSION \"([^\"]+)\"" "\\1" VERSION "${VERSION_MATCH}")
    set(OUTPUT_NAME "${OUTPUT_NAME}-${VERSION}")
endif()

project(${OUTPUT_NAME})

# Set the C++ standard for the entire project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

