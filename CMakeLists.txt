cmake_minimum_required(VERSION 3.24)
project(grpc-adapter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deps (vcpkg CONFIG mode on Windows)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
include(${Protobuf_DIR}/protobuf-generate.cmake)

# --- Protos: generate into source tree: proto/generated ---
set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

add_library(echo_protos STATIC)                         # holds generated code
target_link_libraries(echo_protos PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_include_directories(echo_protos PUBLIC ${PROTO_GEN_DIR})

set(PROTO_FILES ${PROTO_SRC_DIR}/echo.proto)

# protoc for messages (.pb.cc/.pb.h)
protobuf_generate(
  TARGET echo_protos
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  PROTOC_OUT_DIR ${PROTO_GEN_DIR}
  IMPORT_DIRS ${PROTO_SRC_DIR}                          # flatten under generated/
)

# protoc for gRPC stubs (.grpc.pb.cc/.grpc.pb.h)
# Try common vcpkg tool locations; fall back to PATH
find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin
  PATHS
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/grpc"
    "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/grpc"
    "$ENV{VCPKG_ROOT}/installed/x86-windows/tools/grpc"
    "$ENV{VCPKG_ROOT}/installed/arm64-windows/tools/grpc"
)
if(NOT GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found. Ensure gRPC is installed via vcpkg and tools are available.")
endif()

protobuf_generate(
  TARGET echo_protos
  LANGUAGE grpc
  GENERATE_EXTENSIONS .grpc.pb.cc .grpc.pb.h
  PROTOS ${PROTO_FILES}
  PROTOC_OUT_DIR ${PROTO_GEN_DIR}
  IMPORT_DIRS ${PROTO_SRC_DIR}
  PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
)

# --- Your adapter library (headers in include/, sources in src/) ---
add_library(grpc_adapter STATIC
  src/server.cpp
  src/client.cpp
)
target_include_directories(grpc_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(TARGET gRPC::grpc++_reflection)
  target_link_libraries(grpc_adapter PUBLIC gRPC::grpc++ gRPC::grpc++_reflection)
else()
  target_link_libraries(grpc_adapter PUBLIC gRPC::grpc++)
endif()

# --- Examples ---
add_executable(echo_server examples/echo-server.cpp)
target_link_libraries(echo_server PRIVATE grpc_adapter echo_protos)
add_dependencies(echo_server echo_protos grpc_adapter)

add_executable(echo_client examples/echo-client.cpp)
target_link_libraries(echo_client PRIVATE grpc_adapter echo_protos)
add_dependencies(echo_client echo_protos grpc_adapter)
