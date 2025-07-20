cmake_minimum_required(VERSION 3.28)

# Set the project name and version
project(
    ScanForge
    VERSION 1.0.0
    DESCRIPTION "Modern C++23 Point Cloud Processing Library"
    LANGUAGES CXX
)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Let's nicely support folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Testing only available if this is the main app
    # Note this needs to be done in the main CMakeLists
    # since it calls enable_testing, which must be in the
    # main CMakeLists.
    include(CTest)

    # Docs only available if this is the main app
    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()
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

# Note: PreventInSourceBuilds.cmake calls AssureOutOfSourceBuilds() automatically

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
    if(ENABLE_BUILD_WITH_TIME_TRACE)
        target_compile_options(project_options INTERFACE -ftime-trace)
    endif()
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# Note: Cache.cmake sets up compiler cache automatically

# standard compiler warnings
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
enable_sanitizers(project_options)

# enable doxygen if available
# if(Doxygen_FOUND)
#     enable_doxygen("awesome-sidebar")
# endif()

# allow for static analysis options
# enable_static_analyzers()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    if(MSVC)
        # Enable UTF-8 source and execution character sets
        target_compile_options(project_options INTERFACE /utf-8)
        # Enable MSVC-specific optimizations
        target_compile_options(project_options INTERFACE /bigobj)
    endif()
elseif(ANDROID)
    # Android-specific settings
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
elseif(APPLE)
    # macOS-specific settings
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
elseif(UNIX)
    # Linux-specific settings
    target_link_libraries(project_options INTERFACE pthread)
endif()


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
