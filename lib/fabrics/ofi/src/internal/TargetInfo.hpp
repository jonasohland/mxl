#pragma once

#include <iostream>
#include <sys/types.h>
#include "Address.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{
    // TargetInfo encompasses all the information required by an initiator to operate transfers to the given target.
    // In the context of libfabric this means, the fi_addr, all the buffer addresses and sizes, and the remote protection key.
    class TargetInfo
    {
    public:
        explicit TargetInfo();
        explicit TargetInfo(FabricAddress const& fi_addr, Regions const& regions, uint64_t rkey);

        friend std::ostream& operator<<(std::ostream&, TargetInfo const&);
        friend std::istream& operator>>(std::istream&, TargetInfo&);

        [[nodiscard]]
        FabricAddress const& fabricAddress() const noexcept;

        [[nodiscard]]
        Regions const& regions() const noexcept;

        [[nodiscard]]
        uint64_t rkey() const noexcept;

    private:
        FabricAddress _fi_addr;
        Regions _regions;
        uint64_t _rkey;
    };

    std::ostream& operator<<(std::ostream&, TargetInfo const&);
    std::istream& operator>>(std::istream&, TargetInfo const&);
}
