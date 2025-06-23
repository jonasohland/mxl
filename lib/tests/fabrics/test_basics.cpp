#include <catch2/catch_test_macros.hpp>
#include <mxl/fabrics.h>
#include "mxl/mxl.h"

TEST_CASE("Fabrics : Test", "[fabrics]")
{
    auto instance = mxlCreateInstance("/dev/shm/_fabrics_basics", "");

    mxlFabricsInstance fabrics;

    REQUIRE(mxlFabricsCreateInstance(instance, &fabrics) == MXL_STATUS_OK);

    REQUIRE(mxlFabricsDestroyInstance(fabrics) == MXL_STATUS_OK);

    REQUIRE(mxlDestroyInstance(instance) == MXL_STATUS_OK);
}
