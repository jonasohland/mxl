include(GNUInstallDirs)

find_package(PkgConfig)

pkg_check_modules(libfabric REQUIRED IMPORTED_TARGET libfabric)

add_library(mxl-fabrics SHARED)

target_compile_features(mxl-fabrics
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
        mxl
)

target_include_directories(mxl-fabrics
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
)

target_link_libraries(mxl-fabrics
    PRIVATE
        PkgConfig::libfabric
        stduuid
        spdlog::spdlog
        fmt::fmt
        picojson::picojson
)

target_sources(mxl-fabrics
    PRIVATE
        src/fabrics.cpp
        src/internal/FabricsInstance.cpp
        src/internal/Domain.cpp
        src/internal/Address.cpp
        src/internal/MemoryRegion.cpp
)
