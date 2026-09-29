// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include <catch2/catch_test_macros.hpp>
#include "DataLayout.hpp"
#include "ProtocolEgressRMA.hpp"
#include "VerbsDevice.hpp"

using namespace mxl::lib::fabrics::ofi;

TEST_CASE("ofi: maxSampleTransferIovCount counts the header and two fragments per channel", "[ofi][iov]")
{
    REQUIRE(maxSampleTransferIovCount(DataLayout::Continuous{.sampleSize = 4, .channelCount = 1, .bufferLength = 1024}) == 3);
    REQUIRE(maxSampleTransferIovCount(DataLayout::Continuous{.sampleSize = 4, .channelCount = 2, .bufferLength = 1024}) == 5);
    REQUIRE(maxSampleTransferIovCount(DataLayout::Continuous{.sampleSize = 4, .channelCount = 64, .bufferLength = 1024}) == 129);
}

TEST_CASE("ofi: queryVerbsMaxSge returns nothing for an unknown device", "[ofi][iov]")
{
    REQUIRE_FALSE(queryVerbsMaxSge("mxl-no-such-device").has_value());
}
