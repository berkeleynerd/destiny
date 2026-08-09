cmake_minimum_required(VERSION 3.24)
project(DestinyEmbeddedPackageConsumer LANGUAGES CXX)

find_package(carbon-destiny CONFIG REQUIRED)

add_executable(DestinyEmbeddedPackageConsumer main.cpp)
target_compile_features(DestinyEmbeddedPackageConsumer PRIVATE cxx_std_17)
target_link_libraries(DestinyEmbeddedPackageConsumer PRIVATE destinyEmbedded)
set_target_properties(DestinyEmbeddedPackageConsumer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
