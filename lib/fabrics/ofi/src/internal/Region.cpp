// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "Region.hpp"
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <bits/types/struct_iovec.h>
#include "internal/ContinuousFlowData.hpp"
#include "internal/DiscreteFlowData.hpp"
#include "internal/Flow.hpp"
#include "mxl/dataformat.h"
#include "mxl/flow.h"
#include "mxl/mxl.h"
#include "DataLayout.hpp"
#include "Exception.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    Region::Location Region::Location::host() noexcept
    {
        return {Region::Location::Host{}};
    }

    Region::Location Region::Location::cuda(int deviceId) noexcept
    {
        return {Region::Location::Cuda{deviceId}};
    }

    uint64_t Region::Location::id() const noexcept
    {
        return std::visit(
            overloaded{
                [](std::monostate) -> uint64_t { throw Exception::invalidState("Region type is not set"); },
                [](Host const&) -> uint64_t { return 0; }, // Host region has no device ID
                [](Cuda const& cuda) -> uint64_t
                {
                    return static_cast<uint64_t>(cuda._deviceId);
                } // Cuda region returns its device ID
            },
            _inner);
    }

    ::fi_hmem_iface Region::Location::iface() const noexcept
    {
        return std::visit(
            overloaded{
                [](std::monostate) -> ::fi_hmem_iface { throw Exception::invalidState("Region type is not set"); },
                [](Host const&) -> ::fi_hmem_iface { return FI_HMEM_SYSTEM; },
                [](Cuda const&) -> ::fi_hmem_iface
                {
                    return FI_HMEM_CUDA;
                } // Cuda region returns its device ID
            },
            _inner);
    }

    bool Region::Location::isHost() const noexcept
    {
        return std::holds_alternative<Host>(_inner);
    }

    Region::Location Region::Location::fromAPI(mxlFabricsMemoryRegionLocation loc) noexcept
    {
        switch (loc.type)
        {
            case MXL_MEMORY_REGION_TYPE_HOST: return Location::host();
            case MXL_MEMORY_REGION_TYPE_CUDA: return Location::cuda(static_cast<int>(loc.deviceId));
            default:                          throw Exception::invalidArgument("Invalid memory region type");
        }
    }

    std::string Region::Location::toString() const noexcept
    {
        return std::visit(overloaded{[](std::monostate) -> std::string { throw Exception::invalidState("Region type is not set"); },
                              [](Location::Host const&) -> std::string { return "host"; },
                              [&](Location::Cuda const&) -> std::string { return fmt::format("cuda, id={}", id()); }},
            _inner);
    }

    ::iovec const* Region::asIovec() const noexcept
    {
        return &_iovec;
    }

    ::iovec Region::toIovec() const noexcept
    {
        return _iovec;
    }

    // Region implementations
    ::iovec Region::iovecFromRegion(std::uintptr_t base, size_t size) noexcept
    {
        return ::iovec{.iov_base = reinterpret_cast<void*>(base), .iov_len = size};
    }

    ::iovec const* RegionGroup::asIovec() const noexcept
    {
        return _iovecs.data();
    }

    std::vector<::iovec> RegionGroup::iovecsFromGroup(std::vector<Region> const& group) noexcept
    {
        std::vector<::iovec> iovecs;
        std::ranges::transform(group, std::back_inserter(iovecs), [](Region const& reg) { return reg.toIovec(); });
        return iovecs;
    }

    MxlRegions* MxlRegions::fromAPI(mxlRegions regions) noexcept
    {
        return reinterpret_cast<MxlRegions*>(regions);
    }

    mxlRegions MxlRegions::toAPI() noexcept
    {
        return reinterpret_cast<mxlRegions>(this);
    }

    std::vector<RegionGroup> const& MxlRegions::regionGroups() const noexcept
    {
        return _regionGroups;
    }

    DataLayout const& MxlRegions::dataLayout() const noexcept
    {
        return _layout;
    }

    MxlRegions mxlRegionsFromFlow(FlowData& flow)
    {
        static_assert(sizeof(GrainHeader) == 8192,
            "GrainHeader type size changed! The Fabrics API makes assumptions on the memory layout of a flow, please review the code below if the "
            "change is intended!");

        if (mxlIsDiscreteDataFormat(flow.flowInfo()->common.format))
        {
            auto& discreteFlow = static_cast<DiscreteFlowData&>(flow);
            std::vector<RegionGroup> regionGroups;

            for (std::size_t i = 0; i < discreteFlow.grainCount(); ++i)
            {
                auto grain = discreteFlow.grainAt(i);

                auto grainInfoBaseAddr = reinterpret_cast<std::uintptr_t>(discreteFlow.grainAt(i));
                auto grainInfoSize = sizeof(GrainHeader);
                auto grainPayloadSize = grain->header.info.grainSize;

                if (grain->header.info.payloadLocation != MXL_PAYLOAD_LOCATION_HOST_MEMORY)
                {
                    throw Exception::make(MXL_ERR_UNKNOWN,
                        "GPU memory is not currently supported in the Flow API of MXL. Edit the code below when it is supported");
                }

                auto regionGroup = RegionGroup({
                    Region{grainInfoBaseAddr, grainInfoSize + grainPayloadSize, Region::Location::host()},
                });

                regionGroups.emplace_back(std::move(regionGroup));
            }

            return {std::move(regionGroups), DataLayout::fromVideo(false)};
        }
        else if (mxlIsContinuousDataFormat(flow.flowInfo()->common.format))
        {
            auto& continuousFlow = static_cast<ContinuousFlowData&>(flow);
            std::vector<RegionGroup> regionGroups;

            // For the continuous flow, the data layout is a single contiguous buffer
            regionGroups.emplace_back(RegionGroup({
                Region{
                       reinterpret_cast<std::uintptr_t>(continuousFlow.channelData()), continuousFlow.channelDataLength(), Region::Location::host()}
            }));

            return {std::move(regionGroups),
                DataLayout::fromAudio(continuousFlow.channelCount(), continuousFlow.channelBufferLength(), continuousFlow.sampleWordSize())};
        }
        else
        {
            throw Exception::make(MXL_ERR_UNKNOWN, "Unsupported flow fromat {}", flow.flowInfo()->common.format);
        }
    }

    MxlRegions mxlRegionsFromGroups(mxlFabricsMemoryRegionGroup const* groups, size_t count)
    {
        std::vector<RegionGroup> outGroups;
        for (size_t i = 0; i < count; i++)
        {
            std::vector<Region> outRegions;
            auto group = groups[i];
            for (size_t j = 0; j < group.count; j++)
            {
                outRegions.emplace_back(group.regions[j].addr, group.regions[j].size, Region::Location::fromAPI(group.regions[j].loc));
            }
            outGroups.emplace_back(std::move(outRegions));
        }

        return {std::move(outGroups), DataLayout::fromVideo(false)}; // TODO: datalayout struct definition at API level
    }
}
