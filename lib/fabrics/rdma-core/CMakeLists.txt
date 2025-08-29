# SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
#
# SPDX-License-Identifier: Apache-2.0

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(reflectcpp CONFIG REQUIRED)

add_library(mxl-fabrics SHARED)
add_library(mxl-fabrics-static STATIC)

target_compile_features(mxl-fabrics
        PRIVATE
            cxx_std_20
    )

target_compile_features(mxl-fabrics-static
    PRIVATE
        cxx_std_20
)

set_target_properties(mxl-fabrics
        PROPERTIES
            POSITION_INDEPENDENT_CODE    ON
            VISIBILITY_INLINES_HIDDEN    ON
            C_VISIBILITY_PRESET          hidden
            CXX_VISIBILITY_PRESET        hidden
            C_EXTENSIONS                 OFF
            CXX_EXTENSIONS               OFF
            VERSION                      "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
            SOVERSION                    "${PROJECT_VERSION_MAJOR}"
    )

target_link_libraries(mxl-fabrics
    PUBLIC
        mxl-static
)

target_link_libraries(mxl-fabrics-static
    PUBLIC
        mxl-static
)

target_include_directories(mxl-fabrics
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
)

target_include_directories(mxl-fabrics-static
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
)

target_link_libraries(mxl-fabrics
    PRIVATE
        stduuid
        spdlog::spdlog
        fmt::fmt
        picojson::picojson
        reflectcpp::reflectcpp
        rdmacm
        ibverbs
)

target_link_libraries(mxl-fabrics-static
    PRIVATE
        stduuid
        spdlog::spdlog
        fmt::fmt
        picojson::picojson
        reflectcpp::reflectcpp
        rdmacm
        ibverbs
)

set(mxl_fabrics_sources
        src/fabrics.cpp
        src/internal/Address.cpp
        src/internal/CompletionChannel.cpp
        src/internal/CompletionQueue.cpp
        src/internal/ConnectionManagement.cpp
        src/internal/EventChannel.cpp
        src/internal/Exception.cpp
        src/internal/FabricsInstance.cpp
        src/internal/Initiator.cpp
        src/internal/LocalRegion.cpp
        src/internal/MemoryRegion.cpp
        src/internal/ProtectionDomain.cpp
        src/internal/QueuePair.cpp
        src/internal/RCInitiator.cpp
        src/internal/RCTarget.cpp
        src/internal/Region.cpp
        src/internal/RegisteredRegion.cpp
        src/internal/RemoteRegion.cpp
        src/internal/Target.cpp
        src/internal/TargetInfo.cpp
       )

target_sources(mxl-fabrics PRIVATE ${mxl_fabrics_sources})
target_sources(mxl-fabrics-static PRIVATE ${mxl_fabrics_sources})
