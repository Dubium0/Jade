cmake_minimum_required(VERSION 3.10)
project(ImGuiStaticLibrary)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add the ImGui source filesS
file(GLOB IMGUI_SRC
    "*.cpp"
    "backends/imgui_impl_vulkan.cpp"
    "backends/imgui_impl_sdl2.cpp"
)

# Create the static library target
add_library(imgui STATIC ${IMGUI_SRC})

# Include directories
target_include_directories(imgui PUBLIC "" "backends")

# Add required dependencies (GLFW, OpenGL, etc.)
find_package(Vulkan REQUIRED)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

target_link_libraries(imgui PUBLIC SDL2::SDL2)