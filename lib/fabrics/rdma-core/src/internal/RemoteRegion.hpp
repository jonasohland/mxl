// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rfl.hpp>
#include <rfl/Rename.hpp>

namespace mxl::lib::fabrics::rdma_core
{
    struct RemoteRegion
    {
        std::uint64_t addr;
        std::uint64_t rkey;
    };

    namespace rfl_types
    {
        struct RemoteRegionRfl
        {
            rfl::Rename<"addr", std::uint64_t> addr;
            rfl::Rename<"rkey", std::uint64_t> rkey;

            static RemoteRegionRfl from_class(RemoteRegion const& reg)
            {
                return RemoteRegionRfl{.addr = reg.addr, .rkey = reg.rkey};
            }

            [[nodiscard]]
            RemoteRegion to_class() const
            {
                return {.addr = addr.get(), .rkey = rkey.get()};
            }
        };

    };
}

namespace rfl::parsing
{
    namespace rdma = mxl::lib::fabrics::rdma_core;

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, rdma::RemoteRegion, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, rdma::RemoteRegion, rdma::rfl_types::RemoteRegionRfl>
    {};
}
