find_package(glfw3 CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_subdirectory(compute)
add_subdirectory(hello_triangle)
add_subdirectory(scene_render)