#pragma once

#include <chrono>
#include <cstdint>
#include <variant>
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "Endpoint.hpp"
#include "Initiator.hpp"
#include "LocalRegion.hpp"
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
            PassiveEndpoint pep;
        };

        struct Connected
        {
            ActiveEndpoint ep;
            Address addr;
            std::vector<RemoteRegion> regions;
        };

        using State = std::variant<Uninitialized, Idle, Connected>;

    public:
        static std::unique_ptr<RCInitiator> setup(mxlInitiatorConfig const&);

        void addTarget(TargetInfo const&) final;
        void removeTarget(TargetInfo const&) final;
        void transferGrain(std::uint64_t) final;
        bool makeProgress() final;
        bool makeProgressBlocking(std::chrono::steady_clock::duration) final;

    private:
        RCInitiator(State state, std::vector<LocalRegionGroup> localRegions);

    private:
        std::vector<LocalRegionGroup> _localRegions;
        State _state = Uninitialized{};
    };
}
