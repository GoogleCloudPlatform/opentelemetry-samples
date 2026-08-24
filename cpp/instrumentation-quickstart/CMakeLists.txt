# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20)
project(otel_quickstart_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# 2. cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# 3. OpenTelemetry C++ SDK and Exporters
set(WITH_OTLP_HTTP ON CACHE BOOL "Enable OTLP HTTP exporter" FORCE)
set(WITH_OTLP_GRPC OFF CACHE BOOL "Disable OTLP gRPC exporter" FORCE)
set(WITH_OTLP_FILE OFF CACHE BOOL "Disable OTLP file exporter" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable testing" FORCE)
set(WITH_BENCHMARK OFF CACHE BOOL "Disable benchmarks" FORCE)
set(WITH_FUNC_TESTS OFF CACHE BOOL "Disable func tests" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)

find_package(Protobuf REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

FetchContent_Declare(
    opentelemetry_cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG v1.28.0
)
FetchContent_MakeAvailable(opentelemetry_cpp)

add_library(quickstart_lib STATIC
    setup_opentelemetry.cc
    gcp_logging.cc
    http_instrumentation.cc
)

target_include_directories(quickstart_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(quickstart_lib PUBLIC
    opentelemetry_trace
    opentelemetry_metrics
    opentelemetry_exporter_otlp_http
    opentelemetry_exporter_otlp_http_metric
    opentelemetry_resources
    nlohmann_json::nlohmann_json
    httplib::httplib
    protobuf::libprotobuf
    CURL::libcurl
    Threads::Threads
)

add_executable(app main.cc)
target_link_libraries(app PRIVATE quickstart_lib)

# Testing
enable_testing()

add_executable(carrier_test tests/carrier_test.cc)
target_link_libraries(carrier_test PRIVATE quickstart_lib)
add_test(NAME carrier_test COMMAND carrier_test)

add_executable(gcp_logging_test tests/gcp_logging_test.cc)
target_link_libraries(gcp_logging_test PRIVATE quickstart_lib)
add_test(NAME gcp_logging_test COMMAND gcp_logging_test)

add_executable(http_instrumentation_test tests/http_instrumentation_test.cc)
target_link_libraries(http_instrumentation_test PRIVATE quickstart_lib)
add_test(NAME http_instrumentation_test COMMAND http_instrumentation_test)
