#pragma once

#include <memory>
#include <variant>
#include "mxl/fabrics.h"
#include "ConnectionManagement.hpp"
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
            ConnectionManagement cm;
        };

        struct WaitForConnected
        {
            ConnectionManagement cm;
            std::unique_ptr<Target::ImmediateDataLocation> immData;
        };

        struct Connected
        {
            ConnectionManagement cm;
            std::unique_ptr<Target::ImmediateDataLocation> immData;
        };

        using State = std::variant<WaitForConnectionRequest, WaitForConnected, Connected>;

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
