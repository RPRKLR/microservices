cmake_minimum_required(VERSION 3.25)
project(telemetryd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_library(telemetry_proto proto/telemetry.proto)
target_link_libraries(telemetry_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
target_include_directories(telemetry_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET telemetry_proto
    IMPORT_DIRS proto
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET telemetry_proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS proto
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_executable(telemetryd
    src/main.cpp
    src/config.cpp
    src/logging.cpp
    src/ingest_service.cpp
    src/store.cpp
)

target_link_libraries(telemetryd PRIVATE
    telemetry_proto
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

add_executable(ingest_client tools/ingest_client.cpp)
target_link_libraries(ingest_client PRIVATE telemetry_proto)

enable_testing()
add_executable(unit_tests
    tests/store_test.cpp
    tests/config_test.cpp
    src/store.cpp
    src/config.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)
include(GoogleTest)
gtest_discover_tests(unit_tests)

if(MSVC)
    target_compile_options(telemetryd PRIVATE /W4)
else()
    target_compile_options(telemetryd PRIVATE -Wall -Wextra -Wpedantic)
endif()
