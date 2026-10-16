cmake_minimum_required(VERSION 3.10)
project(MonteCarloEstimator LANGUAGES CXX)

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

# The particle-transport code is a library so that the command-line program,
# tests, and future benchmark implementations can share one simulation core.
add_library(particle_transport_core
    src/transport.cpp
)
target_include_directories(particle_transport_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(particle_transport_core PRIVATE /W4)
else()
    target_compile_options(
        particle_transport_core PRIVATE -Wall -Wextra -Wpedantic
    )
endif()

add_executable(particle_transport src/transport_cli.cpp)
target_link_libraries(particle_transport PRIVATE particle_transport_core)

add_executable(transport_benchmark benchmarks/transport_benchmark.cpp)
target_link_libraries(transport_benchmark PRIVATE particle_transport_core)

# Embed the build information needed to interpret benchmark results later.
set(BENCHMARK_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}"
)
string(STRIP "${BENCHMARK_CXX_FLAGS}" BENCHMARK_CXX_FLAGS)
string(REPLACE "\\" "\\\\" BENCHMARK_CXX_FLAGS "${BENCHMARK_CXX_FLAGS}")
string(REPLACE "\"" "\\\"" BENCHMARK_CXX_FLAGS "${BENCHMARK_CXX_FLAGS}")
target_compile_definitions(transport_benchmark PRIVATE
    MONTE_CARLO_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
    MONTE_CARLO_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
    MONTE_CARLO_BUILD_TYPE="$<CONFIG>"
    MONTE_CARLO_CXX_FLAGS="${BENCHMARK_CXX_FLAGS}"
    MONTE_CARLO_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
    MONTE_CARLO_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}"
)

if(MSVC)
    target_compile_options(particle_transport PRIVATE /W4)
    target_compile_options(transport_benchmark PRIVATE /W4)
else()
    target_compile_options(particle_transport PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(
        transport_benchmark PRIVATE -Wall -Wextra -Wpedantic
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(particle_transport_tests
        tests/transport_tests.cpp
    )
    target_link_libraries(
        particle_transport_tests PRIVATE particle_transport_core
    )
    add_test(NAME particle_transport_tests COMMAND particle_transport_tests)
    add_test(
        NAME transport_benchmark_smoke
        COMMAND transport_benchmark
            --particles 10,100
            --repetitions 2
            --warmup 1
            --output ${CMAKE_CURRENT_BINARY_DIR}/transport_benchmark_smoke.csv
    )
endif()

# Link math library (for M_PI constant)
if(UNIX)
    target_link_libraries(monte_carlo_pi m)
endif()

# Set output directory
set_target_properties(
    monte_carlo_pi particle_transport transport_benchmark
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Print configuration info
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
