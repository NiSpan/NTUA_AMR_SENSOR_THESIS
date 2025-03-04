idf_component_register(SRCS "src/mqtt_amr.c"
                       INCLUDE_DIRS "include"
                       REQUIRES mqtt)

                       # Add the main include directory
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/main/include)