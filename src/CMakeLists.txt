# ScanForge Library

set(LIBRARY_NAME scanforge)

# Collect all header files
file(GLOB_RECURSE HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Collect all source files (for now just headers)
set(SOURCE_FILES
    # Add .cpp files here when needed
)

# Create the library
add_library(${LIBRARY_NAME} INTERFACE)

# Set the library properties
target_include_directories(${LIBRARY_NAME} 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Link to project options and warnings (only for build, not exported)
target_link_libraries(${LIBRARY_NAME} 
    INTERFACE 
        $<BUILD_INTERFACE:project_options>
        $<BUILD_INTERFACE:project_warnings>
)

# Set C++23 features
target_compile_features(${LIBRARY_NAME} INTERFACE cxx_std_23)

# Platform-specific libraries
if(WIN32)
    # Windows-specific libraries if needed
elseif(ANDROID)
    # Android-specific libraries if needed
elseif(APPLE)
    # macOS-specific libraries if needed
elseif(UNIX)
    # Linux-specific libraries if needed
    target_link_libraries(${LIBRARY_NAME} INTERFACE pthread)
endif()

# Set library version
set_target_properties(${LIBRARY_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME ScanForge
)

# Create an alias for the library
add_library(ScanForge::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

# Installation
include(GNUInstallDirs)

# Install the headers
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/scanforge
    FILES_MATCHING 
    PATTERN "*.hpp"
    PATTERN "*.h"
)

# Install the library
install(
    TARGETS ${LIBRARY_NAME}
    EXPORT ScanForgeTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export the targets
install(
    EXPORT ScanForgeTargets
    FILE ScanForgeTargets.cmake
    NAMESPACE ScanForge::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanForge
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ScanForgeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ScanForgeConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanForge
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ScanForgeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ScanForgeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ScanForgeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanForge
)

# Add a custom target to show all header files in IDE
add_custom_target(${LIBRARY_NAME}_headers SOURCES ${HEADER_FILES})
