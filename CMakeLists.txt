cmake_minimum_required(VERSION 3.10)

project(VulkanTest LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

find_package(Vulkan REQUIRED)

add_executable(VulkanApp main.cpp)

target_link_libraries(VulkanApp PRIVATE Vulkan::Vulkan)
# Link to the actual SDL3 library.
target_link_libraries(VulkanApp PRIVATE SDL3::SDL3)
