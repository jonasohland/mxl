#include "Region.hpp"
#include <algorithm>
#include <bits/types/struct_iovec.h>
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    // IOVec implementations
    ::iovec IOVec::to_iovec() const
    {
        return ::iovec{.iov_base = reinterpret_cast<void*>(base), .iov_len = size};
    }

    std::ostream& operator<<(std::ostream& os, IOVec const& iov)
    {
        os.write(reinterpret_cast<char const*>(iov.base), sizeof(iov.base));
        os << iov.size;
        return os;
    }

    std::istream& operator>>(std::istream& is, IOVec& iov)
    {
        is.read(reinterpret_cast<char*>(&iov.base), sizeof(iov.base));
        is.read(reinterpret_cast<char*>(&iov.size), sizeof(iov.size));
        return is;
    }

    // Region implementations
    std::vector<::iovec> Region::to_iovec() const noexcept
    {
        std::vector<::iovec> iovec;

        std::ranges::transform(_inner, std::back_inserter(iovec), [](IOVec const& iov) { return iov.to_iovec(); });
        return iovec;
    }

    std::ostream& operator<<(std::ostream& os, Region const& region)
    {
        for (auto const& region : region._inner)
        {
            os << region;
        }
        return os;
    }

    std::istream& operator>>(std::istream& is, Region& region)
    {
        region._inner.clear();

        IOVec iov;
        while (is >> iov)
        {
            region._inner.push_back(iov);
        }
        return is;
    }

    [[nodiscard]]
    Region const& Regions::at(size_t index) const noexcept
    {
        return _inner.at(index);
    }

    [[nodiscard]]
    size_t Regions::size() const
    {
        return _inner.size();
    }

    // Regions implementations
    bool Regions::empty() const noexcept
    {
        return _inner.empty();
    }

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

}
