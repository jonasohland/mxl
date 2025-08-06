#include "RCInitiator.hpp"
#include <chrono>
#include <rfl/json/write.hpp>
#include "internal/Logging.hpp"
#include "Exception.hpp"
#include "RCTarget.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    RCInitiatorTarget::RCInitiatorTarget(Endpoint ep, FabricAddress remote, std::vector<RemoteRegionGroup> rr)
        : _state(Idle{.ep = std::move(ep), .idleSince = std::chrono::steady_clock::time_point{}})
        , _addr(std::move(remote))
        , _regions(std::move(rr))
    {}

    bool RCInitiatorTarget::isIdle() const noexcept
    {
        return std::holds_alternative<Idle>(_state);
    }

    bool RCInitiatorTarget::canEvict() const noexcept
    {
        return std::holds_alternative<Done>(_state);
    }

    bool RCInitiatorTarget::activate(std::shared_ptr<CompletionQueue> const& cq, std::shared_ptr<EventQueue> const& eq)
    {
        _state = std::visit(
            overloaded{
                [&](Idle state) -> State
                {
                    auto idleDuration = std::chrono::steady_clock::now() - state.idleSince;
                    if (idleDuration < std::chrono::seconds(5))
                    {
                        return state;
                    }

                    MXL_INFO("Endpoint has been idle for {}ms, activating",
                        std::chrono::duration_cast<std::chrono::milliseconds>(idleDuration).count());

                    state.ep.bind(eq);
                    state.ep.bind(cq, FI_TRANSMIT);
                    state.ep.connect(_addr);

                    return Connecting{.ep = std::move(state.ep)};
                },
                [](Connecting state) -> State { return state; },
                [](Connected state) -> State { return state; },
                [](Shutdown state) -> State { return state; },
                [](Done state) -> State { return state; },
            },
            std::move(_state));

        return hasPendingWork();
    }

    bool RCInitiatorTarget::consume(Event ev)
    {
        _state = std::visit(
            overloaded{
                [](Idle state) -> State { return state; },
                [&](Connecting state) -> State
                {
                    if (ev.isError())
                    {
                        MXL_ERROR("Failed to connect endpoint: {}", ev.errorString());

                        // Go into the idle state with a new endpoint
                        return Idle{
                            .ep = Endpoint::create(state.ep.domain(), state.ep.id(), state.ep.info()), .idleSince = std::chrono::steady_clock::now()};
                    }
                    else if (ev.isConnected())
                    {
                        MXL_INFO("Endpoint is now connected");

                        return Connected{.ep = std::move(state.ep), .pending = 0};
                    }
                    else if (ev.isShutdown())
                    {
                        MXL_WARN("Received a shutdown event while connecting, going idle");

                        // Go to idle state with a new endpoint
                        return Idle{
                            .ep = Endpoint::create(state.ep.domain(), state.ep.id(), state.ep.info()), .idleSince = std::chrono::steady_clock::now()};
                    }

                    MXL_WARN("Received an unexpected event while establishing a connection");
                    return state;
                },
                [&](Connected state) -> State
                {
                    if (ev.isError())
                    {
                        MXL_WARN("Received an error event in connected state, going idle. Error: {}", ev.errorString());

                        return Idle{
                            .ep = Endpoint::create(state.ep.domain(), state.ep.id(), state.ep.info()), .idleSince = std::chrono::steady_clock::now()};
                    }
                    else if (ev.isShutdown())
                    {
                        MXL_INFO("Remote endpoint has closed the connection");

                        return Idle{
                            .ep = Endpoint::create(state.ep.domain(), state.ep.id(), state.ep.info()), .idleSince = std::chrono::steady_clock::now()};
                    }

                    return state;
                },
                [](Shutdown state) -> State { return state; },
                [](Done state) -> State { return state; },
            },
            std::move(_state));

        return hasPendingWork();
    }

    bool RCInitiatorTarget::consume(Completion completion)
    {
        if (auto error = completion.tryErr(); error)
        {
            handleCompletionError(*error);
        }
        else if (auto data = completion.tryData(); data)
        {
            handleCompletionData(*data);
        }

        return hasPendingWork();
    }

    void RCInitiatorTarget::postTransfer(LocalRegionGroup const& local, uint64_t index)
    {
        if (auto connected = std::get_if<Connected>(&_state); connected != nullptr)
        {
            auto const& remote = _regions[index % _regions.size()];

            connected->ep.write(local, remote, FI_ADDR_UNSPEC, index);
            ++connected->pending;
        }
    }

    void RCInitiatorTarget::handleCompletionData(Completion::Data)
    {
        _state = std::visit(
            overloaded{[](Idle idleState) -> State
                {
                    MXL_WARN("Received a completion event while idle, ignoring.");
                    return idleState;
                },
                [](Connecting connectingState) -> State
                {
                    MXL_WARN("Received a completion event while connecting, ignoring");
                    return connectingState;
                },
                [](Connected connectedState) -> State
                {
                    if (connectedState.pending == 0)
                    {
                        MXL_WARN("Received a completion but no transfer was pending");
                        return connectedState;
                    }

                    --connectedState.pending;

                    return connectedState;
                },
                [](Shutdown shutdownState) -> State
                {
                    MXL_DEBUG("Ignoring completion while shutting down");
                    return shutdownState;
                },
                [](Done doneState) -> State
                {
                    MXL_DEBUG("Ignoring completion after shutdown");
                    return doneState;
                }},
            std::move(_state));
    }

    void RCInitiatorTarget::handleCompletionError(Completion::Error err)
    {
        MXL_ERROR("TODO: handle completion error: {}", err.toString());
    }

    bool RCInitiatorTarget::hasPendingWork() const noexcept
    {
        return std::visit(
            overloaded{
                [](std::monostate) { return false; },
                [](Idle const&) { return false; },
                [](Connecting const&) { return true; },
                [](Connected const& state) { return state.pending > 0; },
                [](Shutdown const&) { return true; },
                [](Done const&) { return false; },
            },
            _state);
    }

    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No provider available");
        }

        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_WRITE | FI_REMOTE_WRITE);

        auto bestFabricInfo = RCTarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto info = bestFabricInfo->owned();

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(fabric);
        auto eq = EventQueue::open(fabric);
        auto cq = CompletionQueue::open(domain);

        auto regionGroups = RegionGroups::fromAPI(config.regions);

        std::vector<RegisteredRegionGroup> registeredRegions;

        for (auto const& group : regionGroups->view())
        {
            std::vector<RegisteredRegion> regRegions;
            for (auto const& region : group.view())
            {
                regRegions.emplace_back(MemoryRegion::reg(domain, region, FI_WRITE), region);
            }

            RegisteredRegionGroup regGroup{std::move(regRegions)};
            registeredRegions.emplace_back(std::move(regGroup));
        }

        struct MakeUniqueEnabler : RCInitiator
        {
            MakeUniqueEnabler(std::shared_ptr<Domain> domain, std::shared_ptr<CompletionQueue> cq, std::shared_ptr<EventQueue> eq,
                std::vector<RegisteredRegionGroup> registeredRegions)
                : RCInitiator(std::move(domain), std::move(cq), std::move(eq), std::move(registeredRegions))
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(std::move(domain), std::move(cq), std::move(eq), std::move(registeredRegions));
    }

    RCInitiator::RCInitiator(std::shared_ptr<Domain> domain, std::shared_ptr<CompletionQueue> cq, std::shared_ptr<EventQueue> eq,
        std::vector<RegisteredRegionGroup> regions)
        : _domain(std::move(domain))
        , _cq(std::move(cq))
        , _eq(std::move(eq))
        , _registeredRegions(regions)
        , _localRegions(toLocal(regions))
    {}

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        auto endpoint = Endpoint::create(_domain, targetInfo.identifier);

        _targets.emplace(endpoint.id(), RCInitiatorTarget{std::move(endpoint), targetInfo.fabricAddress, targetInfo.remoteRegionGroups});
    }

    void RCInitiator::removeTarget(TargetInfo const& targetInfo)
    {}

    void RCInitiator::transferGrain(uint64_t index)
    {
        auto& localRegion = _localRegions[index % _localRegions.size()];
        for (auto& [_, target] : _targets)
        {
            target.postTransfer(localRegion, index);
        }
    }

    bool RCInitiator::pollCQ()
    {
        bool pending = false;

        for (;;)
        {
            auto completion = _cq->tryEntry();
            if (!completion)
            {
                break;
            }

            auto ep = _targets.find(Endpoint::idFromFID(completion->endpoint()));
            if (ep == _targets.end())
            {
                MXL_WARN("Received completion for an unknown endpoint");

                continue;
            }

            pending = ep->second.consume(*completion) || pending;
        }

        return pending;
    }

    bool RCInitiator::pollEQ()
    {
        bool pending = false;

        for (;;)
        {
            auto event = _eq->readEntry();
            if (!event)
            {
                break;
            }

            auto ep = _targets.find(Endpoint::idFromFID(event->fid()));
            if (ep == _targets.end())
            {
                MXL_WARN("Received event for an unknown endpoint");

                continue;
            }

            pending = ep->second.consume(*event) || pending;
        }

        return pending;
    }

    bool RCInitiator::activateIdlePeers()
    {
        bool pending = false;

        for (auto& [_, target] : _targets)
        {
            pending = target.activate(_cq, _eq) || pending;
        }

        return pending;
    }

    void RCInitiator::evictDeadPeers()
    {
        std::erase_if(_targets, [](auto const& item) { return item.second.canEvict(); });
    }

    bool RCInitiator::makeProgress()
    {
        auto pending = activateIdlePeers();
        pending = pollCQ() || pending;
        pending = pollEQ() || pending;

        evictDeadPeers();

        return pending;
    }

    bool RCInitiator::makeProgressBlocking(std::chrono::steady_clock::duration)
    {
        throw Exception::internal("blocking progress not implemented");
    }
}
