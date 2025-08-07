#pragma once

#include <memory>
#include <variant>
#include <vector>
#include "Completion.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "Event.hpp"
#include "Initiator.hpp"
#include "RegisteredRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    class RCInitiatorTarget
    {
    public:
        RCInitiatorTarget(Endpoint, FabricAddress remote, std::vector<RemoteRegionGroup>);

        [[nodiscard]]
        bool hasPendingWork() const noexcept;

        [[nodiscard]]
        bool isIdle() const noexcept;

        [[nodiscard]]
        bool canEvict() const noexcept;

        void shutdown();
        void activate(std::shared_ptr<CompletionQueue> const& cq, std::shared_ptr<EventQueue> const& eq);
        void consume(Event);
        void consume(Completion);
        void postTransfer(LocalRegionGroup const&, uint64_t index);

    private:
        struct Idle
        {
            Endpoint ep;
            std::chrono::steady_clock::time_point idleSince;
        };

        struct Connecting
        {
            Endpoint ep;
        };

        struct Connected
        {
            Endpoint ep;
            std::size_t pending;
        };

        struct Shutdown
        {
            Endpoint ep;
            bool final;
        };

        struct Done
        {};

        void handleCompletionError(Completion::Error);
        void handleCompletionData(Completion::Data);
        void handleConnected();
        void handleShutdown();

        Idle restart(Endpoint const&);

        using State = std::variant<Idle, Connecting, Connected, Shutdown, Done>;

        State _state;
        FabricAddress _addr;
        std::vector<RemoteRegionGroup> _regions;
    };

    class RCInitiator : public Initiator
    {
    public:
        ~RCInitiator() override = default;

        static std::unique_ptr<RCInitiator> setup(mxlInitiatorConfig const& config);

        [[nodiscard]]
        bool hasPendingWork() const noexcept;

        void addTarget(TargetInfo const& targetInfo) final;
        void removeTarget(TargetInfo const& targetInfo) final;
        void transferGrain(uint64_t grainIndex) final;
        bool makeProgress() final;
        bool makeProgressBlocking(std::chrono::steady_clock::duration) final;

    private:
        constexpr static auto EQPollInterval = std::chrono::milliseconds(100);

        RCInitiator(std::shared_ptr<Domain>, std::shared_ptr<CompletionQueue>, std::shared_ptr<EventQueue>, std::vector<RegisteredRegionGroup>);

        void blockOnCQ(std::chrono::steady_clock::duration);
        void pollCQ();
        void pollEQ();

        void activateIdlePeers();
        void evictDeadPeers();

        std::shared_ptr<Domain> _domain;
        std::shared_ptr<CompletionQueue> _cq;
        std::shared_ptr<EventQueue> _eq;

        std::vector<RegisteredRegionGroup> _registeredRegions;
        std::vector<LocalRegionGroup> _localRegions;

        // Default initialized
        std::map<uuids::uuid, RCInitiatorTarget> _targets{};
    };
}
