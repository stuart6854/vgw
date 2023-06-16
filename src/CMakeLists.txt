# Dependencies
if (NOT Vulkan_FOUND)
    find_package(Vulkan REQUIRED OPTIONAL_COMPONENTS shaderc_combined)
endif ()

SET(VGW_TARGET_NAME "vgw")

file(GLOB_RECURSE VGW_SOURCES "*.cpp")
add_library(${VGW_TARGET_NAME} ${VGW_SOURCES})

target_include_directories(${VGW_TARGET_NAME} PRIVATE "." PUBLIC "../include")

target_link_libraries(${VGW_TARGET_NAME} PUBLIC Vulkan::Vulkan Vulkan::shaderc_combined)

set_target_properties(${VGW_TARGET_NAME} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)