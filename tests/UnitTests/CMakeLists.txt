# Unit Tests CMakeLists.txt for ScanForge

# Define test sources
set(TEST_SOURCES
    MainTest.cpp
    LZFCodecTest.cpp
    PointCloudTypesTest.cpp
    PCDWriterTest.cpp
)

# Create test executable
add_executable(scanforgeUnitTests ${TEST_SOURCES})


# Add Catch2 dependency using CPM
CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.4.0
    OPTIONS "CATCH_INSTALL_DOCS OFF" "CATCH_INSTALL_EXTRAS OFF"
)

# Link with the main library and Catch2
target_link_libraries(scanforgeUnitTests 
    PRIVATE 
    scanforge
    Catch2::Catch2WithMain
)

# Include directories
target_include_directories(scanforgeUnitTests 
    PRIVATE 
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/tooling
)

# Set C++23 standard
target_compile_features(scanforgeUnitTests PRIVATE cxx_std_23)

# Enable compiler warnings
target_compile_options(scanforgeUnitTests PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

# Set test working directory to project root so test data paths work
set_target_properties(scanforgeUnitTests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

# Register tests with CTest
include(CTest)

# Use Catch2's test discovery to register individual test cases
if(TARGET Catch2::Catch2WithMain)
    # Include Catch2's CTest integration
    include("${Catch2_SOURCE_DIR}/extras/Catch.cmake")
    catch_discover_tests(scanforgeUnitTests
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
else()
    # Fallback: Add the test executable as a single test for CTest
    add_test(NAME ScanForge_UnitTests COMMAND scanforgeUnitTests)
    set_tests_properties(ScanForge_UnitTests PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    message(WARNING "Catch2 test discovery not available. Registering as single test.")
endif()

# Add custom test data target
add_custom_target(test_data
    COMMENT "Ensuring test data is available"
)

# Copy test data if it exists
if(EXISTS "${PROJECT_SOURCE_DIR}/tests/data")
    add_custom_command(TARGET test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/tests/data"
        "${CMAKE_BINARY_DIR}/tests/data"
        COMMENT "Copying test data to build directory"
    )
endif()

# Make tests depend on test data
add_dependencies(scanforgeUnitTests test_data)
