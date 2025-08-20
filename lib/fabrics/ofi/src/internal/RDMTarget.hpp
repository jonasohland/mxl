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
        RDMTarget(Endpoint endpoint, std::unique_ptr<ImmediateDataLocation> immData);

        Target::ReadResult makeProgress();
        Target::ReadResult makeProgressBlocking(std::chrono::steady_clock::duration timeout);

        template<typename ProgressFunc>
        Target::ReadResult makeProgressImpl(ProgressFunc);

        Endpoint _endpoint;
        std::vector<RegisteredRegionGroup> _regRegions;
        std::unique_ptr<ImmediateDataLocation> _immData;
    };
}
