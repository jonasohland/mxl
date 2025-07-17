#include "Region.hpp"
#include <algorithm>
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{

    Regions* Regions::fromAPI(mxlRegions api) noexcept
    {
        if (!api)
        {
            return nullptr;
        }

        return reinterpret_cast<Regions*>(api);
    }

    ::mxlRegions Regions::toAPI() noexcept
    {
        return reinterpret_cast<::mxlRegions>(this);
    }

    std::ostream& operator<<(std::ostream& os, Region const& region)
    {
        os.write(reinterpret_cast<char const*>(region.base), 8);
        os << region.len;
        return os;
    }

    std::istream& operator>>(std::istream& is, Region& Region)
    {
        is.read(reinterpret_cast<char*>(&Region.base), 8);
        is.read(reinterpret_cast<char*>(&Region.len), sizeof(Region.len));
        return is;
    }

    ::iovec Region::to_iovec() const noexcept
    {
        ::iovec iov;
        iov.iov_base = base;
        iov.iov_len = len;
        return iov;
    }

    std::ostream& operator<<(std::ostream& os, Regions const& regions)
    {
        for (auto const& region : regions._inner)
        {
            os << region;
        }
        return os;
    }

    std::istream& operator>>(std::istream& is, Regions& regions)
    {
        regions._inner.clear();

        Region region;
        while (is >> region)
        {
            regions._inner.push_back(region);
        }
        return is;
    }

    std::vector<::iovec> Regions::to_iovec() const noexcept
    {
        std::vector<::iovec> iovecs;
        std::ranges::transform(_inner, std::back_inserter(iovecs), [](Region const& region) { return region.to_iovec(); });
        return iovecs;
    }
}