cmake_minimum_required(VERSION 3.24)
project(fabric-grpc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(FABRIC_BUILD_APPS "Build sample apps" ON)
option(FABRIC_BUILD_TESTS "Build tests" ON)

add_subdirectory(proto)
add_subdirectory(modules/core)

if(FABRIC_BUILD_APPS)
  add_subdirectory(apps/echo)
endif()

if(FABRIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()