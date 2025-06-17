# FindgRPC.cmake
# Finds the gRPC library
#
# This will define the following variables:
#
#   gRPC_FOUND        - True if the system has gRPC
#   gRPC_INCLUDE_DIRS - gRPC include directory
#   gRPC_LIBRARIES    - gRPC libraries
#   gRPC_VERSION      - gRPC version

include(FindPackageHandleStandardArgs)

# Try to find gRPC using pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_GRPC QUIET grpc++)
    pkg_check_modules(PC_GRPCPP QUIET grpc++)
endif()

# Find headers
find_path(gRPC_INCLUDE_DIR
    NAMES grpc++/grpc++.h
    PATHS
        ${PC_GRPC_INCLUDEDIR}
        /usr/include
        /usr/local/include
        /opt/grpc/include
)

# Find libraries
find_library(gRPC_LIBRARY
    NAMES grpc++
    PATHS
        ${PC_GRPC_LIBDIR}
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
        /opt/grpc/lib
)

find_library(gRPC_REFLECTION_LIBRARY
    NAMES grpc++_reflection
    PATHS
        ${PC_GRPC_LIBDIR}
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
        /opt/grpc/lib
)

# Find protoc-gen-grpc-cpp plugin
find_program(gRPC_CPP_PLUGIN
    NAMES grpc_cpp_plugin
    PATHS
        /usr/bin
        /usr/local/bin
        /opt/grpc/bin
        ${PC_GRPC_PREFIX}/bin
        $ENV{HOME}/.local/bin
)

# Set gRPC_FOUND
set(gRPC_INCLUDE_DIRS ${gRPC_INCLUDE_DIR})
set(gRPC_LIBRARIES ${gRPC_LIBRARY} ${gRPC_REFLECTION_LIBRARY})

# Handle version
if(PC_GRPC_VERSION)
    set(gRPC_VERSION ${PC_GRPC_VERSION})
endif()

# Make gRPC_CPP_PLUGIN optional
if(NOT gRPC_CPP_PLUGIN)
    message(STATUS "gRPC C++ plugin not found. Some features may be limited.")
endif()

find_package_handle_standard_args(gRPC
    REQUIRED_VARS
        gRPC_LIBRARY
        gRPC_REFLECTION_LIBRARY
        gRPC_INCLUDE_DIR
    VERSION_VAR gRPC_VERSION
)

mark_as_advanced(
    gRPC_INCLUDE_DIR
    gRPC_LIBRARY
    gRPC_REFLECTION_LIBRARY
    gRPC_CPP_PLUGIN
) 