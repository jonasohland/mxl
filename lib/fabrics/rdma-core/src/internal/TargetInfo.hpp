// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rfl.hpp>
#include <uuid.h>
#include <fmt/format.h>
#include <rfl/Field.hpp>
#include <sys/types.h>
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    // TargetInfo contains all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    struct TargetInfo
    {
        TargetInfo(Address addr, std::vector<RemoteRegion> regions)
            : addr(std::move(addr))
            , remoteRegions(std::move(regions))
        {}

        static TargetInfo* fromAPI(mxlTargetInfo api) noexcept;

        [[nodiscard]]
        ::mxlTargetInfo toAPI() noexcept;

        Address addr;
        std::vector<RemoteRegion> remoteRegions;
    };

    // namespace rlf_types
    //{
    struct TargetInfoRfl
    {
        rfl::Rename<"addr", std::string> addr;
        rfl::Rename<"regions", std::vector<RemoteRegion>> regions;

        static TargetInfoRfl from_class(TargetInfo const& ti)
        {
            rfl::Rename<"regions", std::vector<RemoteRegion>> rflRegions;
            std::ranges::copy(ti.remoteRegions.begin(), ti.remoteRegions.end(), std::back_inserter(rflRegions.value()));
            return TargetInfoRfl{.addr = ti.addr.toString(), .regions = rflRegions};
        }

        [[nodiscard]]
        TargetInfo to_class() const
        {
            return {Address::fromString(addr.get()), regions.get()};
        };
    };

    //}
}

namespace rfl::parsing
{
    namespace rdma = mxl::lib::fabrics::rdma_core;

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, rdma::TargetInfo, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, rdma::TargetInfo, rdma::TargetInfoRfl>
    {};

}
