// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#include <uuid.h>
#include <catch2/catch_test_macros.hpp>
#include <mxl/flow.h>
#include <mxl/mxl.h>
#include "Utils.hpp"

TEST_CASE_PERSISTENT_FIXTURE(mxl::tests::mxlDomainFixture, "Flow readers / writers caching", "[instance]")
{
    auto const opts = "{}";
    auto instance = mxlCreateInstance(domain.string().c_str(), opts);
    REQUIRE(instance != nullptr);

    mxlFlowWriter audioWriter;
    mxlFlowWriter videoWriter;
    mxlFlowWriter metaWriter;
    mxlFlowConfigInfo configInfo;
    auto flowDef = mxl::tests::readFile("data/audio_flow.json");
    REQUIRE(mxlCreateFlowWriter(instance, flowDef.c_str(), opts, &audioWriter, &configInfo) == MXL_STATUS_OK);
    auto const audioFlowId = uuids::to_string(configInfo.common.id);
    flowDef = mxl::tests::readFile("data/v210_flow.json");
    REQUIRE(mxlCreateFlowWriter(instance, flowDef.c_str(), opts, &videoWriter, &configInfo) == MXL_STATUS_OK);
    auto const videoFlowId = uuids::to_string(configInfo.common.id);
    flowDef = mxl::tests::readFile("data/data_flow.json");
    REQUIRE(mxlCreateFlowWriter(instance, flowDef.c_str(), opts, &metaWriter, &configInfo) == MXL_STATUS_OK);
    auto const metaFlowId = uuids::to_string(configInfo.common.id);

    REQUIRE(audioFlowId != videoFlowId);
    REQUIRE(audioFlowId != metaFlowId);
    REQUIRE(videoFlowId != metaFlowId);

    REQUIRE(audioWriter != videoWriter);
    REQUIRE(audioWriter != metaWriter);
    REQUIRE(videoWriter != metaWriter);

    mxlFlowReader audioReader;
    REQUIRE(mxlCreateFlowReader(instance, audioFlowId.c_str(), "", &audioReader) == MXL_STATUS_OK);
    REQUIRE(audioReader != nullptr);
    mxlFlowReader audioReader2;
    REQUIRE(mxlCreateFlowReader(instance, audioFlowId.c_str(), "", &audioReader2) == MXL_STATUS_OK);
    REQUIRE(audioReader2 != nullptr);
    mxlFlowReader videoReader;
    REQUIRE(mxlCreateFlowReader(instance, videoFlowId.c_str(), "", &videoReader) == MXL_STATUS_OK);
    REQUIRE(videoReader != nullptr);
    mxlFlowReader metaReader;
    REQUIRE(mxlCreateFlowReader(instance, metaFlowId.c_str(), "", &metaReader) == MXL_STATUS_OK);
    REQUIRE(metaReader != nullptr);

    REQUIRE(audioReader == audioReader2);
    REQUIRE(audioReader != videoReader);
    REQUIRE(audioReader != metaReader);
    REQUIRE(videoReader != metaReader);

    REQUIRE(static_cast<void*>(audioReader) != static_cast<void*>(audioWriter));
    REQUIRE(static_cast<void*>(audioReader) != static_cast<void*>(videoWriter));
    REQUIRE(static_cast<void*>(audioReader) != static_cast<void*>(metaWriter));

    REQUIRE(mxlReleaseFlowReader(instance, audioReader) == MXL_STATUS_OK);
    REQUIRE(mxlReleaseFlowReader(instance, audioReader2) == MXL_STATUS_OK);
    REQUIRE(mxlReleaseFlowReader(instance, videoReader) == MXL_STATUS_OK);
    REQUIRE(mxlReleaseFlowReader(instance, metaReader) == MXL_STATUS_OK);

    REQUIRE(mxlReleaseFlowWriter(instance, audioWriter) == MXL_STATUS_OK);
    REQUIRE(mxlReleaseFlowWriter(instance, videoWriter) == MXL_STATUS_OK);
    REQUIRE(mxlReleaseFlowWriter(instance, metaWriter) == MXL_STATUS_OK);

    REQUIRE(mxlDestroyInstance(instance) == MXL_STATUS_OK);
}
