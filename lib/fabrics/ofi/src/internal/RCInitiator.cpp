// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "RCInitiator.hpp"
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <uuid.h>
#include <rdma/fabric.h>
#include <rfl/json/write.hpp>
#include "internal/Logging.hpp"
#include "BounceBufferContinuous.hpp"
#include "DataLayout.hpp"
#include "Domain.hpp"
#include "Exception.hpp"
#include "Protocol.hpp"
#include "Region.hpp"
#include "RemoteRegion.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    RCInitiatorEndpoint::RCInitiatorEndpoint(Endpoint ep, DataLayout const& layout, TargetInfo info)
        : _state(Idle{.ep = std::move(ep), .idleSince = std::chrono::steady_clock::time_point{}})
        , _layout(std::move(layout))
        , _info(std::move(info))
    {}

    bool RCInitiatorEndpoint::isIdle() const noexcept
    {
        return std::holds_alternative<Idle>(_state);
    }

    bool RCInitiatorEndpoint::canEvict() const noexcept
    {
        return std::holds_alternative<Done>(_state);
    }

    bool RCInitiatorEndpoint::hasPendingWork() const noexcept
    {
        return std::visit(
            overloaded{
                [](std::monostate) { return false; },   // Something went wrong with this target, but there is probably no work to do.
                [](Idle const&) { return true; },       // An idle target means there is no work to do right now.
                [](Connecting const&) { return true; }, // In the connecting state, the target is waiting for a connected event.
                [](Connected const& state)
                { return state.pending > 0; }, // While connected, a target has pending work when there are transfers that have not yet completed.
                [](Shutdown const&) { return true; }, // In the shutdown state, the target is waiting for a FI_SHUTDOWN event.
                [](Done const&) { return false; },    // In the done state, there is no pending work.
            },
            _state);
    }

    void RCInitiatorEndpoint::shutdown()
    {
        _state = std::visit(
            overloaded{
                [](Idle) -> State
                {
                    MXL_INFO("Shutdown requested while waiting to activate, aborting.");
                    return Done{};
                },
                [](Connecting) -> State
                {
                    MXL_INFO("Shutdown requested while trying to connect, aborting.");
                    return Done{};
                },
                [](Connected state) -> State
                {
                    MXL_INFO("Shutting down");
                    state.ep->shutdown();

                    return Shutdown{.ep = std::move(*state.ep)};
                },
                [](Shutdown) -> State
                {
                    MXL_WARN("Another shutdown was requested while trying to shut down, aborting.");
                    return Done{};
                },
                [](Done state) -> State { return state; },
            },
            std::move(_state));
    }

    void RCInitiatorEndpoint::activate(std::shared_ptr<CompletionQueue> const& cq, std::shared_ptr<EventQueue> const& eq)
    {
        _state = std::visit(
            overloaded{
                [&](Idle state) -> State
                {
                    // If the target is in the idle state for more than 5 seconds, it will be restarted.
                    auto idleDuration = std::chrono::steady_clock::now() - state.idleSince;
                    if (idleDuration < std::chrono::seconds(5))
                    {
                        return state;
                    }

                    MXL_INFO("Endpoint has been idle for {}ms, activating",
                        std::chrono::duration_cast<std::chrono::milliseconds>(idleDuration).count());

                    // The endpoint in an idle target is always fresh and thus needs to be bound the the queues.
                    state.ep.bind(eq);
                    state.ep.bind(cq, FI_TRANSMIT);

                    // Transition into the connecting state
                    state.ep.connect(_info.fabricAddress);
                    return Connecting{.ep = std::move(state.ep)};
                },
                [](Connecting state) -> State { return state; },
                [](Connected state) -> State { return state; },
                [](Shutdown state) -> State { return state; },
                [](Done state) -> State { return state; },
            },
            std::move(_state));
    }

    void RCInitiatorEndpoint::consume(Event ev)
    {
        _state = std::visit(
            overloaded{
                [](Idle state) -> State { return state; }, // Nothing to do when idle.
                [&](Connecting state) -> State
                {
                    if (ev.isError())
                    {
                        MXL_ERROR("Failed to connect endpoint: {}", ev.errorString());

                        // Go into the idle state with a new endpoint
                        return restart(state.ep);
                    }
                    else if (ev.isConnected())
                    {
                        MXL_INFO("Endpoint is now connected");

                        auto sharedEp = std::make_shared<Endpoint>(std::move(state.ep));

                        return Connected{.ep = sharedEp, .proto = selectProtocol(*sharedEp, _layout, _info), .pending = 0};
                    }
                    else if (ev.isShutdown())
                    {
                        MXL_WARN("Received a shutdown event while connecting, going idle");

                        // Go to idle state with a new endpoint
                        return restart(state.ep);
                    }

                    MXL_WARN("Received an unexpected event while establishing a connection");
                    return state;
                },
                [&](Connected state) -> State
                {
                    if (ev.isError())
                    {
                        MXL_WARN("Received an error event in connected state, going idle. Error: {}", ev.errorString());

                        return restart(*state.ep);
                    }
                    else if (ev.isShutdown())
                    {
                        MXL_INFO("Remote endpoint has closed the connection");

                        return restart(*state.ep);
                    }

                    return state;
                },
                [&](Shutdown state) -> State
                {
                    if (ev.isShutdown())
                    {
                        MXL_INFO("Shutdown complete");
                        return Done{};
                    }
                    else if (ev.isError())
                    {
                        MXL_ERROR("Received an error while shutting down: {}", ev.errorString());
                        return Done{};
                    }
                    else
                    {
                        MXL_ERROR("Received an unexpected event while shutting down");
                        return state;
                    }
                },
                [](Done state) -> State { return state; },
            },
            std::move(_state));
    }

    void RCInitiatorEndpoint::consume(Completion completion)
    {
        if (auto error = completion.tryErr(); error)
        {
            handleCompletionError(*error);
        }
        else if (auto data = completion.tryData(); data)
        {
            handleCompletionData(*data);
        }
    }

    void RCInitiatorEndpoint::postTransfer(LocalRegion const& local, std::uint64_t index)
    {
        if (auto connected = std::get_if<Connected>(&_state); connected != nullptr)
        {
            // Post a write work item to the endpoint and increment the pending counter. When the write is complete, a completion will be posted to
            // the completion queue, after which the counter will be decremented again if the target is still in the connected state.
            connected->pending += connected->proto->transferGrain(local, index, 0); // TODO: sliceIndex
        }
    }

    void RCInitiatorEndpoint::postTransfer(LocalRegionGroup const& localRegionGroup, std::uint64_t partialHeadIndex, std::size_t count)
    {
        if (auto connected = std::get_if<Connected>(&_state); connected != nullptr)
        {
            connected->pending += connected->proto->transferSamples(localRegionGroup, partialHeadIndex, count);
        }
    }

    void RCInitiatorEndpoint::handleCompletionData(Completion::Data)
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

    void RCInitiatorEndpoint::handleCompletionError(Completion::Error err)
    {
        MXL_ERROR("TODO: handle completion error: {}", err.toString());
    }

    RCInitiatorEndpoint::Idle RCInitiatorEndpoint::restart(Endpoint const& old)
    {
        return Idle{.ep = Endpoint::create(old.domain(), old.id(), old.info()), .idleSince = std::chrono::steady_clock::now()};
    }

    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No provider available");
        }

        uint64_t caps = FI_RMA | FI_WRITE | FI_REMOTE_WRITE;
        caps |= config.deviceSupport ? FI_HMEM : 0;

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, provider.value(), caps, FI_EP_MSG);

        if (fabricInfoList.begin() == fabricInfoList.end())
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto info = *fabricInfoList.begin();
        auto fabric = Fabric::open(info);
        auto domain = Domain::open(fabric);

        if (config.regions == nullptr)
        {
            throw Exception::invalidArgument("config.regions must not be null");
        }

        auto const mxlRegions = MxlRegions::fromAPI(config.regions);
        domain->registerRegions(mxlRegions->regions(), FI_WRITE);

        auto eq = EventQueue::open(fabric);
        auto cq = CompletionQueue::open(domain);

        struct MakeUniqueEnabler : RCInitiator
        {
            MakeUniqueEnabler(std::shared_ptr<Domain> domain, std::shared_ptr<CompletionQueue> cq, std::shared_ptr<EventQueue> eq,
                DataLayout dataLayout)
                : RCInitiator(std::move(domain), std::move(cq), std::move(eq), std::move(dataLayout))
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(std::move(domain), std::move(cq), std::move(eq), mxlRegions->dataLayout());
    }

    RCInitiator::RCInitiator(std::shared_ptr<Domain> domain, std::shared_ptr<CompletionQueue> cq, std::shared_ptr<EventQueue> eq,
        DataLayout dataLayout)
        : _domain(std::move(domain))
        , _cq(std::move(cq))
        , _eq(std::move(eq))
        , _dataLayout(std::move(dataLayout))
        , _localRegions(_domain->localRegions())
    {}

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        _targets.emplace(targetInfo.id, RCInitiatorEndpoint{Endpoint::create(_domain, targetInfo.id), _dataLayout, targetInfo});
    }

    void RCInitiator::removeTarget(TargetInfo const& targetInfo)
    {
        if (auto it = _targets.find(targetInfo.id); it != _targets.end())
        {
            it->second.shutdown();
        }
        else
        {
            throw Exception::notFound("Target with id {} not found", targetInfo.id);
        }
    }

    void RCInitiator::transferGrain(uint64_t index)
    {
        if (_localRegions.empty())
        {
            throw Exception::internal("Attempted to transfer grains with no region registered.");
        }
        if (!_dataLayout.isVideo())
        {
            throw Exception::internal("transferGrain called, but the data layout for that endpoint is not video.");
        }

        // Find the local region in which the grain with this index is stored.
        auto& localRegion = _localRegions[index % _localRegions.size()];

        // Post a transfer work item to all targets. If the target is not in a connected state
        // this is a no-op.
        for (auto& [_, target] : _targets)
        {
            target.postTransfer(localRegion, index);
        }
    }

    void RCInitiator::transferSamples(std::uint64_t headIndex, std::size_t count)
    {
        if (_localRegions.empty())
        {
            throw Exception::internal("Attempted to transfer samples with no region registered.");
        }
        if (!_dataLayout.isAudio())
        {
            throw Exception::internal("transferSamples called, but the data layout for that endpoint is not audio.");
        }
        auto audioDataLayout = _dataLayout.asAudio();

        auto sgList = scatterGatherList(audioDataLayout, headIndex, count, _localRegions.front());

        // Do the post the transfer to each targets
        for (auto& [_, target] : _targets)
        {
            target.postTransfer(sgList,
                headIndex % audioDataLayout.samplesPerChannel,
                count); // TODO: pass the actual headIndex and count and add them to scatter-gather list
        }
    }

    bool RCInitiator::hasPendingWork() const noexcept
    {
        // Check if any of the targets have pending work.
        for (auto& [_, target] : _targets)
        {
            if (target.hasPendingWork())
            {
                return true;
            }
        }

        return false;
    }

    void RCInitiator::activateIdleEndpoints()
    {
        // Call the activate function on all endpoints. This is a no-op when the endpoint is not idle
        // and there is probably not that many of them.
        for (auto& [_, target] : _targets)
        {
            target.activate(_cq, _eq);
        }
    }

    void RCInitiator::evictDeadEndpoints()
    {
        std::erase_if(_targets, [](auto const& item) { return item.second.canEvict(); });
    }

    void RCInitiator::blockOnCQ(std::chrono::system_clock::duration timeout)
    {
        // A zero timeout would cause the queue to block indefinetly, which
        // is not our documented behaviour.
        if (timeout == std::chrono::milliseconds::zero())
        {
            // So just behave exactly like the non-blocking variant.
            makeProgress();
            return;
        }

        for (;;)
        {
            auto completion = _cq->readBlocking(timeout);
            if (!completion)
            {
                return;
            }

            // Find the endpoint that this completion was generated from
            auto ep = _targets.find(Endpoint::idFromFID(completion->fid()));
            if (ep == _targets.end())
            {
                MXL_WARN("Received completion for an unknown endpoint");
            }

            return ep->second.consume(*completion);
        }
    }

    void RCInitiator::pollCQ()
    {
        for (;;)
        {
            auto completion = _cq->read();
            if (!completion)
            {
                break;
            }

            // Find the endpoint this completion was generated from.
            auto ep = _targets.find(Endpoint::idFromFID(completion->fid()));
            if (ep == _targets.end())
            {
                MXL_WARN("Received completion for an unknown endpoint");

                continue;
            }

            ep->second.consume(*completion);
        }
    }

    void RCInitiator::pollEQ()
    {
        for (;;)
        {
            auto event = _eq->read();
            if (!event)
            {
                break;
            }

            // Find the endpoint this event was generated from.
            auto ep = _targets.find(Endpoint::idFromFID(event->fid()));
            if (ep == _targets.end())
            {
                MXL_WARN("Received event for an unknown endpoint");

                continue;
            }

            ep->second.consume(*event);
        }
    }

    bool RCInitiator::makeProgress()
    {
        // Activate any peers that might be idle and waiting for activation.
        activateIdleEndpoints();

        // Poll the completion and event queue once and process pending events.
        pollCQ();
        pollEQ();

        // Evict any peers that are dead and no longer will make progress.
        evictDeadEndpoints();

        return hasPendingWork();
    }

    bool RCInitiator::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        // If the timeout is less than our maintainance interval, just check all the queues once, execute all maintainance tasks once
        // and block on the completion queue for the rest of the time.
        if (timeout < EQPollInterval)
        {
            makeProgress();
            blockOnCQ(timeout);
            return hasPendingWork();
        }

        auto deadline = std::chrono::steady_clock::now() + timeout;

        for (;;)
        {
            // If there is nothing more to do, return control to the caller.
            if (!hasPendingWork())
            {
                return false;
            }

            // Poll all queues, execute all maintainance actions
            makeProgress();

            // Calculate the remaining time until the user wants the blocking function to return. If there is no time left
            // (millisecond precision) return right away.
            auto timeUntilDeadline = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            if (timeUntilDeadline <= decltype(timeUntilDeadline){0})
            {
                return hasPendingWork();
            }

            // Block on the completion queue until a completion arrives, or the interval timeout occurrs.
            blockOnCQ(std::min(EQPollInterval, timeUntilDeadline));
        }

        return hasPendingWork();
    }
}
