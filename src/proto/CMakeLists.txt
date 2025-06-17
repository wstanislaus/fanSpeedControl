include(FindProtobuf)
include(FindgRPC)

set(PROTO_FILES
    mcu_simulator.proto
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

if(NOT GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found!")
endif()

protobuf_generate_cpp(
  PROTO_SRCS
  PROTO_HDRS
  ${PROTO_FILES}
)

protobuf_generate_grpc_cpp(
  GRPC_SRCS
  GRPC_HDRS
  PROTOS ${PROTO_FILES}
)

message(STATUS "PROTO_SRCS: ${PROTO_SRCS}")
message(STATUS "PROTO_HDRS: ${PROTO_HDRS}")
message(STATUS "GRPC_SRCS: ${GRPC_SRCS}")
message(STATUS "GRPC_HDRS: ${GRPC_HDRS}")

add_library(proto
    ${PROTO_SRCS} ${PROTO_HDRS}
    ${GRPC_SRCS} ${GRPC_HDRS}
)

target_include_directories(proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

target_link_libraries(proto PUBLIC
    ${Protobuf_LIBRARIES}
    ${gRPC_LIBRARIES}
)