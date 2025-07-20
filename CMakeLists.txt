cmake_minimum_required(VERSION 4.0)
# CMake 4.0+ provides full C++23 modules support

# Set the project name and version
project(
    ScanForge
    VERSION 1.0.0
    DESCRIPTION "Modern C++23 Point Cloud Processing Library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++23 modules support (when enabled in future versions)
# set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
# set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Testing only available if this is the main app
    # Note this needs to be done in the main CMakeLists
    # since it calls enable_testing, which must be in the
    # main CMakeLists.
    include(CTest)
endif()

# Include additional CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our custom cmake modules
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
    add_subdirectory(tests)
endif()

# Display build configuration summary
include(DisplayStatus)
display_status()
