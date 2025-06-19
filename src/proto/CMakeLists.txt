include(FindProtobuf)
include(FindgRPC)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

if(NOT GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found!")
endif()

# Create separate targets for each proto file
# MCU Simulator Proto
protobuf_generate_cpp(
  MCU_PROTO_SRCS
  MCU_PROTO_HDRS
  mcu_simulator.proto
)

protobuf_generate_grpc_cpp(
  MCU_GRPC_SRCS
  MCU_GRPC_HDRS
  PROTOS mcu_simulator.proto
)

add_library(mcu_simulator_proto
    ${MCU_PROTO_SRCS} ${MCU_PROTO_HDRS}
    ${MCU_GRPC_SRCS} ${MCU_GRPC_HDRS}
)

target_include_directories(mcu_simulator_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

target_link_libraries(mcu_simulator_proto PUBLIC
    ${Protobuf_LIBRARIES}
    ${gRPC_LIBRARIES}
)

# Fan Control System Proto
protobuf_generate_cpp(
  FAN_PROTO_SRCS
  FAN_PROTO_HDRS
  fan_control_system.proto
)

protobuf_generate_grpc_cpp(
  FAN_GRPC_SRCS
  FAN_GRPC_HDRS
  PROTOS fan_control_system.proto
)

add_library(fan_control_system_proto
    ${FAN_PROTO_SRCS} ${FAN_PROTO_HDRS}
    ${FAN_GRPC_SRCS} ${FAN_GRPC_HDRS}
)

target_include_directories(fan_control_system_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

target_link_libraries(fan_control_system_proto PUBLIC
    ${Protobuf_LIBRARIES}
    ${gRPC_LIBRARIES}
)