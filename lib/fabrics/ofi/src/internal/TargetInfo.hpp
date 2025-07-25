#pragma once

#include <rfl.hpp>
#include <uuid.h>
#include <rfl/Field.hpp>
#include <sys/types.h>
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    // TargetInfo contains all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    struct TargetInfo
    {
        TargetInfo() = default;

        TargetInfo(FabricAddress fabricAddress, std::vector<RemoteRegionGroup> regions, uuids::uuid identifier = uuids::uuid_system_generator{}())
            : fabricAddress(std::move(fabricAddress))
            , remoteRegionGroups(std::move(regions))
            , identifier(identifier)
        {}

        static TargetInfo* fromAPI(mxlTargetInfo api) noexcept;

        [[nodiscard]]
        ::mxlTargetInfo toAPI() noexcept;

        FabricAddress fabricAddress;
        std::vector<RemoteRegionGroup> remoteRegionGroups;
        uuids::uuid identifier;
    };

    // namespace rlf_types
    //{
    struct TargetInfoRfl
    {
        rfl::Rename<"fabricAddress", FabricAddressRfl> fabricAddress;
        rfl::Rename<"regions", std::vector<RemoteRegionGroup>> regions;
        rfl::Rename<"identifier", std::string> identifier;

        static TargetInfoRfl from_class(TargetInfo const& ti)
        {
            rfl::Rename<"regions", std::vector<RemoteRegionGroup>> rflRegions;
            for (auto const& reg : ti.remoteRegionGroups) // TODO: use std::ranges::transform instead:
            {
                rflRegions.value().emplace_back(reg);
            }

            return TargetInfoRfl{.fabricAddress = FabricAddressRfl::from_class(ti.fabricAddress),
                .regions = rflRegions,
                .identifier = uuids::to_string(ti.identifier)};
        }

        [[nodiscard]]
        TargetInfo to_class() const
        {
            return {fabricAddress.get().to_class(), regions.get(), uuids::uuid::from_string(identifier.get()).value()};
        };
    };

    //}
}

namespace rfl::parsing
{
    namespace ofi = mxl::lib::fabrics::ofi;

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, ofi::TargetInfo, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, ofi::TargetInfo, ofi::TargetInfoRfl>
    {};

}
