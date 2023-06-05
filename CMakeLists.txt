cmake_minimum_required(VERSION 3.25)
project(vgw LANGUAGES CXX)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(examples)
