#pragma once

#include <cstdint>
#include <iostream>
#include <uuid.h>
#include <sys/types.h>
#include "Address.hpp"
#include "MemoryRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct RemoteRegion
    {
        static RemoteRegion fromRegisteredRegion(RegisteredRegion& reg)
        {
            return {reg.region().firstBaseAddress(), reg.rkey()};
        }

        RemoteRegion() = default;

        RemoteRegion(uint64_t addr, uint64_t rkey)
            : addr(addr)
            , rkey(rkey)
        {}

        // TODO: needs to be serialized/deserialized
        friend std::ostream& operator<<(std::ostream&, RemoteRegion const&);
        friend std::istream& operator>>(std::istream&, RemoteRegion&);

        uint64_t addr;
        uint64_t rkey;
    };

    // TargetInfo encompasses all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    struct TargetInfo
    {
        TargetInfo() = default;

        TargetInfo(FabricAddress fabricAddress, std::vector<RemoteRegion> regions, uuids::uuid identifier = uuids::uuid_system_generator{}())
            : fabricAddress(std::move(fabricAddress))
            , regions(std::move(regions))
            , _identifier(std::move(identifier))
        {}

        static TargetInfo* fromAPI(mxlTargetInfo api) noexcept;

        [[nodiscard]]
        std::string identifier() const noexcept;

        [[nodiscard]]
        ::mxlTargetInfo toAPI() noexcept;

        friend std::ostream& operator<<(std::ostream&, TargetInfo const&);
        friend std::istream& operator>>(std::istream&, TargetInfo&);

        FabricAddress fabricAddress;
        std::vector<RemoteRegion> regions;
        uuids::uuid _identifier;
    };

}
