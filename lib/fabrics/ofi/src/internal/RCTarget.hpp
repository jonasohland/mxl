#pragma once

#include <memory>
#include <variant>
#include "mxl/fabrics.h"
#include "Endpoint.hpp"
#include "PassiveEndpoint.hpp"
#include "RegisteredRegion.hpp"
#include "Target.hpp"

namespace mxl::lib::fabrics::ofi
{
    // Realiable+Connected Target implementation
    class RCTarget : public Target
    {
    private:
        struct WaitForConnectionRequest
        {
            PassiveEndpoint pep;
        };

        struct WaitForConnection
        {
            Endpoint ep;
        };

        struct Connected
        {
            Endpoint ep;
            std::unique_ptr<ImmediateDataLocation> immData;
        };

        using State = std::variant<WaitForConnectionRequest, WaitForConnection, Connected>;

    public:
        static std::pair<std::unique_ptr<RCTarget>, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const&);

        Target::ReadResult read() final;
        Target::ReadResult readBlocking(std::chrono::steady_clock::duration timeout) final;

    private:
        RCTarget(std::shared_ptr<Domain> domain, std::vector<RegisteredRegionGroup> regions, PassiveEndpoint pep);

        Target::ReadResult makeProgress();
        Target::ReadResult makeProgressBlocking(std::chrono::steady_clock::duration timeout);

        std::shared_ptr<Domain> _domain;
        std::vector<RegisteredRegionGroup> _regRegions;

        State _state;
    };
}
