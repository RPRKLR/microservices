cmake_minimum_required(VERSION 3.25)
project(telemetryd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

add_executable(telemetryd
    src/main.cpp
    src/config.cpp
    src/logging.cpp
)

target_link_libraries(telemetryd PRIVATE
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

if(MSVC)
    target_compile_options(telemetryd PRIVATE /W4)
else()
    target_compile_options(telemetryd PRIVATE -Wall -Wextra -Wpedantic)
endif()
