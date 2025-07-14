#pragma once

#include <optional>
#include "FIInfo.hpp"
#include "Target.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{

    class RMATarget : public Target
    {
    public:
        static std::optional<FIInfoView> findBestFabric(FIInfoList& available, mxlFabricsProvider providerPreference);

        [[nodiscard]]
        TargetInfo getInfo() const noexcept override;
    };
}
