if (NOT TARGET Catch2::Catch2WithMain)
    find_package(Catch2 REQUIRED)
endif()

add_executable(mxl-fabrics-tests)

target_compile_features(mxl-fabrics-tests
        PRIVATE
            cxx_std_20
    )

set_target_properties(mxl-fabrics-tests
        PROPERTIES
            POSITION_INDEPENDENT_CODE    ON
            VISIBILITY_INLINES_HIDDEN    ON
            C_VISIBILITY_PRESET          hidden
            CXX_VISIBILITY_PRESET        hidden
            C_EXTENSIONS                 OFF
            CXX_EXTENSIONS               OFF
    )

target_sources(mxl-fabrics-tests
    PRIVATE
        test_basics.cpp
)

target_link_libraries(mxl-fabrics-tests
    PRIVATE
        mxl
        mxl-fabrics
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(mxl-fabrics-tests)


