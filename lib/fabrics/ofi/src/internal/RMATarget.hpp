#pragma once

#include "Target.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class RMATarget : public Target
    {
    public:
        [[nodiscard]]
        TargetInfo getInfo() const noexcept override;
    };
}
