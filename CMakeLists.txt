cmake_minimum_required(VERSION 3.10)
project(MonteCarloPiEstimator)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags for optimization
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Create executable
add_executable(monte_carlo_pi monte_carlo_pi.cpp)

# Link math library (for M_PI constant)
if(UNIX)
    target_link_libraries(monte_carlo_pi m)
endif()

# Set output directory
set_target_properties(monte_carlo_pi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Print configuration info
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}") 