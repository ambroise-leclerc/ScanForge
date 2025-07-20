cmake_minimum_required(VERSION 4.0)
project(
    ScanForge
    VERSION 1.0.0
    DESCRIPTION "Modern C++23 Point Cloud Processing Library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(StandardProjectSettings)
include(PreventInSourceBuilds)
include(Cache)
include(Sanitizers)
include(StaticAnalyzers)
include(CompilerWarnings)
include(CPM)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)
add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)
enable_sanitizers(project_options)
# allow for static analysis options
# enable_static_analyzers()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


find_package(Threads REQUIRED)

# Add the library subdirectory
add_subdirectory(src)

# Add the executable subdirectory
add_subdirectory(app)

# Testing
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()

# Display build configuration summary
include(DisplayStatus)
display_status()
