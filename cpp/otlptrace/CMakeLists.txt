cmake_minimum_required(VERSION 3.20)
project(otlp_trace_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

include(FetchContent)

set(WITH_OTLP_GRPC ON CACHE BOOL "Enable OTLP gRPC exporter" FORCE)
set(WITH_OTLP_HTTP OFF CACHE BOOL "Disable OTLP HTTP exporter" FORCE)
set(WITH_OTLP_FILE OFF CACHE BOOL "Disable OTLP file exporter" FORCE)
set(WITH_OTLP_GRPC_CREDENTIAL_PREVIEW ON CACHE BOOL "Enable gRPC credential preview" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable testing" FORCE)
set(WITH_BENCHMARK OFF CACHE BOOL "Disable benchmarks" FORCE)
set(WITH_FUNC_TESTS OFF CACHE BOOL "Disable func tests" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)

add_compile_definitions(ENABLE_OTLP_GRPC_CREDENTIAL_PREVIEW)

# [START opentelemetry_otlp_grpc_auth_deps]
find_package(gRPC REQUIRED)
# [END opentelemetry_otlp_grpc_auth_deps]
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

# [START opentelemetry_otlp_grpc_deps]
FetchContent_Declare(
    opentelemetry_cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG v1.28.0
)
FetchContent_MakeAvailable(opentelemetry_cpp)
# [END opentelemetry_otlp_grpc_deps]

add_library(otlptrace_lib STATIC otlptrace.cc)
target_include_directories(otlptrace_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(otlptrace_lib PUBLIC
    opentelemetry_trace
    opentelemetry_exporter_otlp_grpc
    opentelemetry_resources
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
    Threads::Threads
)

add_executable(otlptrace example.cc)
target_link_libraries(otlptrace PRIVATE otlptrace_lib)

enable_testing()
add_executable(otlptrace_test tests/otlptrace_test.cc)
target_link_libraries(otlptrace_test PRIVATE otlptrace_lib)
add_test(NAME otlptrace_test COMMAND otlptrace_test)
