set(APP_NAME scanforgeCli)

set(SOURCE_FILES
    main.cpp
)


CPMAddPackage( NAME CLI11 GITHUB_REPOSITORY CLIUtils/CLI11 VERSION 2.4.2 OPTIONS "CLI11_BUILD_TESTS OFF" "CLI11_BUILD_EXAMPLES OFF")

add_executable(${APP_NAME} ${SOURCE_FILES})
target_link_libraries(${APP_NAME} PRIVATE ScanForge::scanforge CLI11::CLI11 project_options  project_warnings)
set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME scanforge RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    if(MSVC)
        set_target_properties(${APP_NAME} PROPERTIES
            WIN32_EXECUTABLE FALSE  # Console application
        )
    endif()
elseif(ANDROID)
    # Android-specific settings
    # Note: For Android, you might want to create a shared library instead
    # and use JNI to interface with Java/Kotlin code
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE  # Command line tool, not a bundle
    )
elseif(UNIX)
    # Linux-specific settings
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS ${APP_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Create a symbolic link for easier access (Unix-like systems only)
if(UNIX AND NOT ANDROID)
    install(CODE "
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_INSTALL_FULL_BINDIR}/scanforge
            ${CMAKE_INSTALL_FULL_BINDIR}/sf
        )
    ")
endif()
