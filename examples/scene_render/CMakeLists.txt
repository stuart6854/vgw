
set(VGW_EXAMPLE_NAME "vgw_example_scene_render")

add_executable(${VGW_EXAMPLE_NAME} main.cpp)

target_link_libraries(${VGW_EXAMPLE_NAME} PRIVATE vgw glfw tinyobjloader::tinyobjloader)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/geometry.vert"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "geometry.vert" "${CMAKE_CURRENT_BINARY_DIR}/geometry.vert"
        DEPENDS "geometry.vert"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying vertex shader."
)
add_custom_target("${VGW_EXAMPLE_NAME}_vertex_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/geometry.vert")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_vertex_shader")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/geometry.frag"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "geometry.frag" "${CMAKE_CURRENT_BINARY_DIR}/geometry.frag"
        DEPENDS "geometry.frag"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying fragment shader."
)
add_custom_target("${VGW_EXAMPLE_NAME}_fragment_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/geometry.frag")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_fragment_shader")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "viking_room.obj" "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj"
        DEPENDS "viking_room.obj"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying model."
)
add_custom_target("${VGW_EXAMPLE_NAME}_model" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj")
add_dependencies(${VGW_EXAMPLE_NAME} "${VGW_EXAMPLE_NAME}_model")