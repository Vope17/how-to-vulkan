# 1. 將最低版本提升至 3.14，因為我們需要使用 FetchContent 模組
cmake_minimum_required(VERSION 3.14)

project(VulkanTest LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# 2. 引入 FetchContent 來自動下載並設定 tinyobjloader
include(FetchContent)
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        release
)
FetchContent_MakeAvailable(tinyobjloader)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)


find_package(Vulkan REQUIRED)

# 3. 將專門用來編譯 VMA 實作的 .cpp 檔案加入執行檔中
add_executable(VulkanApp main.cpp)


# 4. 移除原本全域的 VMA_IMPLEMENTATION，改為開啟 Vulkan 1.3 支援
target_compile_definitions(VulkanApp PRIVATE VMA_IMPLEMENTATION)

target_link_libraries(VulkanApp PRIVATE Vulkan::Vulkan)
target_link_libraries(VulkanApp PRIVATE SDL3::SDL3)
# 5. 連結 tinyobjloader 靜態庫
target_link_libraries(VulkanApp PRIVATE tinyobjloader)
