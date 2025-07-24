#pragma once

#include <cstdint>
#include <rfl.hpp>
#include <uuid.h>
#include <rfl/Field.hpp>
#include <sys/types.h>
#include "Address.hpp"
#include "MemoryRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct RemoteRegion
    {
        static RemoteRegion fromRegisteredRegion(RegisteredRegion& reg)

        {
            return {.addr = reg.region().firstBaseAddress(), .rkey = reg.rkey()};
        }

        uint64_t addr;
        uint64_t rkey;
    };

    struct RemoteRegionRfl
    {
        rfl::Field<"addr", uint64_t> addr;
        rfl::Field<"rkey", uint64_t> rkey;

        static RemoteRegionRfl from_class(RemoteRegion const& rr)
        {
            return RemoteRegionRfl{.addr = rr.addr, .rkey = rr.rkey};
        }

        [[nodiscard]]
        RemoteRegion to_class() const
        {
            return RemoteRegion{.addr = addr.get(), .rkey = rkey.get()};
        }
    };

    // TargetInfo encompasses all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    struct TargetInfo
    {
        TargetInfo() = default;

        TargetInfo(FabricAddress fabricAddress, std::vector<RemoteRegion> regions, uuids::uuid identifier = uuids::uuid_system_generator{}())
            : fabricAddress(std::move(fabricAddress))
            , regions(std::move(regions))
            , _identifier(uuids::to_string(identifier))
        {}

        static TargetInfo* fromAPI(mxlTargetInfo api) noexcept;

        [[nodiscard]]
        std::string identifier() const noexcept;

        [[nodiscard]]
        ::mxlTargetInfo toAPI() noexcept;

        FabricAddress fabricAddress;
        std::vector<RemoteRegion> regions;
        std::string _identifier;
    };

    struct TargetInfoRfl
    {
        rfl::Rename<"fabricAddress", FabricAddressRfl> fabricAddress;
        rfl::Rename<"regions", std::vector<RemoteRegionRfl>> regions;
        rfl::Rename<"identifier", std::string> identifier;

        static TargetInfoRfl from_class(TargetInfo const& ti)
        {
            rfl::Rename<"regions", std::vector<RemoteRegionRfl>> rflRegions;
            for (auto const& reg : ti.regions) // TODO: use std::ranges::transform instead:
            {
                rflRegions.value().emplace_back(RemoteRegionRfl::from_class(reg));
            }

            return TargetInfoRfl{
                .fabricAddress = FabricAddressRfl::from_class(ti.fabricAddress), .regions = rflRegions, .identifier = ti.identifier()};
        }

        [[nodiscard]]
        TargetInfo to_class() const
        {
            std::vector<RemoteRegion> origRegions;
            for (auto const& reg : regions.get())
            {
                origRegions.emplace_back(reg.to_class());
            }

            return {fabricAddress.get().to_class(), origRegions, uuids::uuid::from_string(identifier.get()).value()};
        };
    };
}

namespace rfl::parsing
{
    namespace ofi = mxl::lib::fabrics::ofi;

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, ofi::RemoteRegion, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, ofi::RemoteRegion, ofi::RemoteRegionRfl>
    {};

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, ofi::TargetInfo, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, ofi::TargetInfo, ofi::TargetInfoRfl>
    {};

}
