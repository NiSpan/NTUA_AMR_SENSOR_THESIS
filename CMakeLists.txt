# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AMR_Sensor_thesis)

# Add the include directory
include_directories(${CMAKE_SOURCE_DIR}/main/include)

# Add the components directory
list(APPEND EXTRA_COMPONENT_DIRS components)

project(AMR_Sensor_thesis)