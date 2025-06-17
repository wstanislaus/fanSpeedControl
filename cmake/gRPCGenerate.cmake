# gRPCGenerate.cmake
# Defines functions for generating gRPC code from proto files

# Find the gRPC C++ plugin
find_program(gRPC_CPP_PLUGIN
    NAMES grpc_cpp_plugin
    PATHS /usr/bin
    DOC "The gRPC C++ plugin for protoc"
)

if(NOT gRPC_CPP_PLUGIN)
    message(FATAL_ERROR "gRPC C++ plugin not found. Please install the gRPC development package.")
endif()

# Function to generate gRPC C++ code from proto files
function(protobuf_generate_grpc_cpp SRCS HDRS)
    if(NOT ARGN)
        message(SEND_ERROR "Error: protobuf_generate_grpc_cpp() called without any proto files")
        return()
    endif()

    if(NOT gRPC_CPP_PLUGIN)
        message(FATAL_ERROR "gRPC C++ plugin not found. Please install the gRPC development package.")
    endif()

    set(${SRCS})
    set(${HDRS})

    # Process arguments
    set(_proto_files)
    set(_collecting_protos FALSE)
    
    foreach(arg ${ARGN})
        if(arg STREQUAL "PROTOS")
            set(_collecting_protos TRUE)
        elseif(_collecting_protos)
            list(APPEND _proto_files ${arg})
        endif()
    endforeach()

    foreach(FIL ${_proto_files})
        get_filename_component(FIL_DIR ${FIL} DIRECTORY)
        get_filename_component(FIL_WE ${FIL} NAME_WE)

        list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.cc")
        list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.h")

        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.cc"
                   "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.h"
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN}
                 --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                 --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                 -I${CMAKE_CURRENT_SOURCE_DIR}
                 ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}
            DEPENDS ${FIL}
            COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
            VERBATIM)
    endforeach()

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction() 