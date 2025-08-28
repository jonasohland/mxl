#include "TargetInfo.hpp"
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::rdma_core
{

    TargetInfo* TargetInfo::fromAPI(mxlTargetInfo api) noexcept
    {
        return reinterpret_cast<TargetInfo*>(api);
    }

    ::mxlTargetInfo TargetInfo::toAPI() noexcept
    {
        return reinterpret_cast<mxlTargetInfo>(this);
    }

}
