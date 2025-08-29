#pragma once

#include <chrono>
#include <cstdint>
#include <variant>
#include "mxl/fabrics.h"
#include "ConnectionManagement.hpp"
#include "Initiator.hpp"
#include "LocalRegion.hpp"
#include "QueueHelpers.hpp"
#include "RemoteRegion.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class RCInitiator : public Initiator
    {
    private:
        struct Uninitialized
        {};

        struct Idle
        {
            ConnectionManagement cm;
        };

        struct WaitForAddrResolved
        {
            ConnectionManagement cm;
            std::vector<RemoteRegion> regions;
        };

        struct WaitForRouteResolved
        {
            ConnectionManagement cm;
            std::vector<RemoteRegion> regions;
        };

        struct WaitConnection
        {
            ConnectionManagement cm;
            std::vector<RemoteRegion> regions;
        };

        struct Connected

        {
            ConnectionManagement cm;
            std::vector<RemoteRegion> regions;
        };

        struct Done
        {
            ConnectionManagement cm;
        };

        using State = std::variant<Uninitialized, Idle, WaitForAddrResolved, WaitForRouteResolved, WaitConnection, Connected, Done>;

    public:
        static std::unique_ptr<RCInitiator> setup(mxlInitiatorConfig const&);

        void addTarget(TargetInfo const&) final;
        void removeTarget(TargetInfo const&) final;
        void transferGrain(std::uint64_t) final;
        bool makeProgress() final;
        bool makeProgressBlocking(std::chrono::steady_clock::duration) final;

    private:
        RCInitiator(State state, std::vector<LocalRegionGroup> localRegions);

        template<QueueReadMode>
        bool makeProgressInternal(std::chrono::steady_clock::duration);

    private:
        std::vector<LocalRegionGroup> _localRegions;
        State _state = Uninitialized{};
        size_t _pendingTransfer = 0;
    };
}
