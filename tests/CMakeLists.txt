# ScanForge Tests

# Only build tests if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    
    # Find required packages for testing
    find_package(Threads REQUIRED)
    
    # Add test subdirectories
    add_subdirectory(UnitTests)
    
    # Create a custom target for running all tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS scanforgeUnitTests
        COMMENT "Running all tests"
    )
    
endif()
