#pragma once

#include <memory>
#include "mxl/fabrics.h"
#include "Endpoint.hpp"
#include "RegisteredRegion.hpp"
#include "Target.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class RDMTarget : public Target
    {
    public:
        static std::pair<std::unique_ptr<RDMTarget>, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const&);

        Target::ReadResult read() final;
        Target::ReadResult readBlocking(std::chrono::steady_clock::duration timeout) final;

    private:
        RDMTarget(std::unique_ptr<Endpoint> endpoint, std::vector<RegisteredRegionGroup> regions);

        Target::ReadResult makeProgress();
        Target::ReadResult makeProgressBlocking(std::chrono::steady_clock::duration timeout);

        std::unique_ptr<Endpoint> _endpoint;
        std::vector<RegisteredRegionGroup> _regRegions;
    };
}
