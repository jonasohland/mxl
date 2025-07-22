#include "TargetInfo.hpp"
#include <utility>
#include "Address.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::ostream& operator<<(std::ostream& os, TargetInfo const& targetInfo)
    {
        os << targetInfo.fabricAddress << targetInfo.remoteRegions;

        return os;
    }

    std::istream& operator>>(std::istream& is, TargetInfo& targetInfo)
    {
        is >> targetInfo.fabricAddress >> targetInfo.remoteRegions;

        return is;
    }

    TargetInfo* TargetInfo::fromAPI(mxlTargetInfo api) noexcept
    {
        return reinterpret_cast<TargetInfo*>(api);
    }

    ::mxlTargetInfo TargetInfo::toAPI() noexcept
    {
        return reinterpret_cast<mxlTargetInfo>(this);
    }

}
