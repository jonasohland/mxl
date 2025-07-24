#include "TargetInfo.hpp"
#include <uuid/uuid.h>

namespace mxl::lib::fabrics::ofi
{

    std::string TargetInfo::identifier() const noexcept
    {
        return _identifier;
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
