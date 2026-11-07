cmake_minimum_required(VERSION 3.10.0)
project(SylvonixShader VERSION 0.1.0 LANGUAGES C CXX)

add_executable(SylvonixShader main.cpp slangcomp.cpp svshaderFile.cpp)

# --- Slang from Vulkan SDK ---
find_library(Slang_LIBRARY
    NAMES slang slang-compiler
    HINTS "$ENV{VULKAN_SDK}/lib"
    REQUIRED
)

target_include_directories(SylvonixShader PRIVATE
    "$ENV{VULKAN_SDK}/include"
)

target_link_libraries(SylvonixShader PRIVATE ${Slang_LIBRARY})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SylvonixShader PROPERTY CXX_STANDARD 20)
endif()
