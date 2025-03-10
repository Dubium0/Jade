cmake_minimum_required(VERSION 3.10)
project(StbStaticLibrary)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add the STB source files
file(GLOB STB_SRC
    "src/*.cpp"
)

# Create the static library target
add_library(stb STATIC ${STB_SRC})

# Include directories
target_include_directories(stb PUBLIC "" )
