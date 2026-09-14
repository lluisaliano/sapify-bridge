# Minimum required
cmake_minimum_required(VERSION 3.15)

set(LANGUAGES CXX)
# Version 0.1.0
project(sapify-bridge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(sapify-bridge
  src/main.cpp
  src/config/Config.cpp
  src/services/ShopifyClient.cpp
)

target_include_directories(sapify-bridge
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(sapify-bridge
  PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wnon-virtual-dtor)

target_link_libraries(sapify-bridge
  PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json)
