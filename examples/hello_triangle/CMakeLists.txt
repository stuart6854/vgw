
set(VGW_EXAMPLE_NAME "vgw_example_hello_triangle")

add_executable(${VGW_EXAMPLE_NAME} main.cpp)

target_link_libraries(${VGW_EXAMPLE_NAME} PRIVATE vgw glfw)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/triangle.vert"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "triangle.vert" "${CMAKE_CURRENT_BINARY_DIR}/triangle.vert"
        DEPENDS "triangle.vert"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying vertex shader."
)
add_custom_target("${VGW_EXAMPLE_NAME}_vertex_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/triangle.vert")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_vertex_shader")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/triangle.frag"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "triangle.frag" "${CMAKE_CURRENT_BINARY_DIR}/triangle.frag"
        DEPENDS "triangle.frag"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying fragment shader."
)
add_custom_target("${VGW_EXAMPLE_NAME}_fragment_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/triangle.frag")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_fragment_shader")