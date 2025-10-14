// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include <catch2/catch_test_macros.hpp>
#include <rfl/json/read.hpp>
#include <rfl/json/write.hpp>
#include "TargetInfo.hpp"

using namespace mxl::lib::fabrics::ofi;

TEST_CASE("TargetInfo empty", "[ofi::TargetInfo]")
{
    TargetInfo empty;
    REQUIRE(empty.remoteRegions.empty());
    REQUIRE(empty.fabricAddress.size() == 0);
}

TEST_CASE("TargetInfo deserialize/serialize", "[ofi::TargetInfo]")
{
    auto input =
        R"({"fabricAddress":{"addr":"AgAjg38AAAEAAAAAAAAAAA=="},"regions":[{"addr":0,"len":2496512,"rkey":12490884954606633550},{"addr":0,"len":2496512,"rkey":8202674608102871622}],"identifier":"1995225397354848055"})";

    auto info = rfl::json::read<TargetInfo>(input);

    REQUIRE(info->fabricAddress.toBase64() == "AgAjg38AAAEAAAAAAAAAAA==");
    REQUIRE(info->id == 1995225397354848055U);
    REQUIRE(info->remoteRegions.size() == 2);

    auto region = info->remoteRegions[0];
    REQUIRE(region.addr == 0U);
    REQUIRE(region.len == 2496512U);
    REQUIRE(region.rkey == 12490884954606633550U);

    region = info->remoteRegions[1];
    REQUIRE(region.addr == 0U);
    REQUIRE(region.len == 2496512U);
    REQUIRE(region.rkey == 8202674608102871622U);

    // Let's re-serialize the TargetInfo
    auto output = rfl::json::write(info);
    REQUIRE(output == input);
}
