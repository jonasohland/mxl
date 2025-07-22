#pragma once

#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include "Address.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct RemoteRegion
    {
        uint64_t addr;
        uint64_t rkey;
        // TODO: needs to be serialized/deserialized
        friend std::ostream& operator<<(std::ostream&, RemoteRegion const&);
        friend std::istream& operator>>(std::istream&, RemoteRegion&);
    };

    class RemoteRegions
    {
    public:
        RemoteRegions() = default;

        [[nodiscard]]
        RemoteRegion const& at(size_t index) const
        {
            return _regions.at(index);
        }

        [[nodiscard]]
        size_t size() const noexcept
        {
            return _regions.size();
        }

        friend std::ostream& operator<<(std::ostream&, RemoteRegions const&);
        friend std::istream& operator>>(std::istream&, RemoteRegions&);

    private:
        // TODO: needs to be serialized/deserialized
        std::vector<RemoteRegion> _regions;
    };

    // TargetInfo encompasses all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    struct TargetInfo

    {
        friend std::ostream& operator<<(std::ostream&, TargetInfo const&);
        friend std::istream& operator>>(std::istream&, TargetInfo&);

        static TargetInfo* fromAPI(mxlTargetInfo api) noexcept;

        [[nodiscard]]
        ::mxlTargetInfo toAPI() noexcept;

        std::string asIdentifier() const noexcept;

        FabricAddress fabricAddress;
        RemoteRegions remoteRegions;
    };

    std::ostream& operator<<(std::ostream&, TargetInfo const&);
    std::istream& operator>>(std::istream&, TargetInfo const&);
}
