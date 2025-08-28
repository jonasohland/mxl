#pragma once

#include <variant>
#include "mxl/fabrics.h"
#include "Endpoint.hpp"
#include "QueueHelpers.hpp"
#include "Target.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class RCTarget : public Target
    {
    private:
        struct WaitForConnectionRequest
        {
            PassiveEndpoint pep;
        };

        struct Connected
        {
            ActiveEndpoint ep;
            Target::ImmediateDataLocation _immData;
        };

        using State = std::variant<WaitForConnectionRequest, Connected>;

    public:
        static std::pair<std::unique_ptr<RCTarget>, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const&);

        Target::ReadResult read() final;
        Target::ReadResult readBlocking(std::chrono::steady_clock::duration timeout) final;

    private:
        RCTarget(State);

        template<QueueReadMode>
        Target::ReadResult makeProgress(std::chrono::steady_clock::duration timeout);

    private:
        State _state;
    };
}
