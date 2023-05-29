
set(VGW_EXAMPLE_NAME "vgw_example_compute")

add_executable(${VGW_EXAMPLE_NAME} main.cpp)

target_link_libraries(${VGW_EXAMPLE_NAME} PRIVATE vgw)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compute.comp"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "compute.comp" "${CMAKE_CURRENT_BINARY_DIR}/compute.comp"
        DEPENDS "compute.comp"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying compute shader."
)
add_custom_target("${VGW_EXAMPLE_NAME}_compute_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compute.comp")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_compute_shader")