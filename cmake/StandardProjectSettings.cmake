# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
             "Debug"
             "Release"
             "MinSizeRel"
             "RelWithDebInfo")
endif()

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "Generate compile_commands.json for clang based tools")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

if(ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT
    result
    OUTPUT
    output)
  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(SEND_ERROR "IPO is not supported: ${output}")
  endif()
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif(MSVC)
  # Enable colored diagnostics for MSVC (available in VS 2019 16.4+)
  if(MSVC_VERSION GREATER_EQUAL 1924)
    add_compile_options(/diagnostics:column)
  endif()
  
  # MSVC-specific optimizations
  add_compile_options(
    # Release optimizations
    $<$<CONFIG:Release>:/O2>      # Maximum optimization for speed
    $<$<CONFIG:Release>:/Ob2>     # Inline function expansion
    $<$<CONFIG:Release>:/Ot>      # Favor fast code over small code
    $<$<CONFIG:Release>:/GL>      # Whole program optimization
    
    # Debug settings
    $<$<CONFIG:Debug>:/ZI>        # Enable Edit and Continue debugging
    $<$<CONFIG:Debug>:/Od>        # Disable optimization for debugging
    
    # General settings
    /MP                           # Multi-processor compilation
  )
  
  # Link-time optimizations for Release builds
  add_link_options(
    $<$<CONFIG:Release>:/LTCG>    # Link-time code generation
    $<$<CONFIG:Release>:/OPT:REF> # Remove unreferenced functions/data
    $<$<CONFIG:Release>:/OPT:ICF> # Identical COMDAT folding
  )
else()
  message(STATUS "No colored compiler diagnostic set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
endif()