#include "TargetInfo.hpp"
#include <utility>
#include <uuid/uuid.h>
#include "Address.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::ostream& operator<<(std::ostream& os, RemoteRegion const& region)
    {
        os << region.addr << region.rkey;

        return os;
    }

    std::istream& operator>>(std::istream& is, RemoteRegion& region)
    {
        is >> region.addr >> region.rkey;

        return is;
    }

    std::string TargetInfo::identifier() const noexcept
    {
        return uuids::to_string(_identifier);
    }

    TargetInfo* TargetInfo::fromAPI(mxlTargetInfo api) noexcept
    {
        return reinterpret_cast<TargetInfo*>(api);
    }

    ::mxlTargetInfo TargetInfo::toAPI() noexcept
    {
        return reinterpret_cast<mxlTargetInfo>(this);
    }

    std::ostream& operator<<(std::ostream& os, TargetInfo const& targetInfo)
    {
        os << targetInfo.fabricAddress;

        for (auto const& region : targetInfo.regions)
        {
            os << region;
        }

        return os;
    }

    std::istream& operator>>(std::istream& is, TargetInfo& targetInfo)
    {
        is >> targetInfo.fabricAddress;

        RemoteRegion region;
        while (is >> region)
        {
            targetInfo.regions.emplace_back(std::move(region));
        }

        return is;
    }

}
