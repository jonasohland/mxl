#include "RCInitiator.hpp"
#include <chrono>
#include <variant>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "ConnectionManagement.hpp"
#include "Exception.hpp"
#include "LocalRegion.hpp"
#include "QueueHelpers.hpp"
#include "QueuePair.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto bindAddr = Address{config.endpointAddress.node, config.endpointAddress.service};

        auto cm = ConnectionManagement::create();
        cm.bind(bindAddr);

        cm.createProtectionDomain();
        cm.pd().registerRegionGroups(*RegionGroups::fromAPI(config.regions),
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ); // Local read access is always enabled for the MR.

        MXL_INFO("Success to register memory");

        auto localRegions = cm.pd().localRegionGroups();
        for (auto const& region : localRegions)
        {
            MXL_INFO("LocalRegion -> addr=0x{:x} len={} lkey=0x{:x} ", region.sgl()->addr, region.sgl()->length, region.sgl()->lkey);
        }

        // Helper struct to enable the std::make_unique function to access the private constructor of this class
        struct MakeUniqueEnabler : RCInitiator
        {
            MakeUniqueEnabler(State state, std::vector<LocalRegionGroup> localRegions)
                : RCInitiator(std::move(state), std::move(localRegions))
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(Idle{std::move(cm)}, std::move(localRegions));
    }

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        MXL_INFO("Add Target {}", targetInfo.addr.toString());

        if (auto state = std::get_if<Idle>(&_state); state)
        {
            auto dstAddr = targetInfo.addr;

            // Prepare connection
            state->cm.resolveAddr(dstAddr, std::chrono::seconds(2));

            _state = WaitForAddrResolved{.cm = std::move(state->cm), .regions = targetInfo.remoteRegions};
        }
        else
        {
            throw Exception::internal("Attempt to add a target when not in Idle state.");
        }
    }

    void RCInitiator::removeTarget(TargetInfo const&)
    {
        if (auto state = std::get_if<Connected>(&_state); state)
        {
            _state = Done{std::move(state->cm)};
            MXL_INFO("Transition to Done state");
        }
        else
        {
            throw Exception::internal("Attempted to remove target when not in Connected state");
        }
    }

    void RCInitiator::transferGrain(std::uint64_t grainIndex)
    {
        if (auto state = std::get_if<Connected>(&_state); state)
        {
            auto& remote = state->regions[grainIndex % state->regions.size()];
            auto& local = _localRegions[grainIndex % _localRegions.size()].front();

            state->cm.write(grainIndex, local, remote);
            _pendingTransfer++;
        }
    }

    template<QueueReadMode queueReadMode>
    bool RCInitiator::makeProgressInternal(std::chrono::steady_clock::duration timeout)
    {
        bool status = true;

        _state = std::visit(
            overloaded{[](std::monostate) -> State
                { throw Exception::invalidState("Initiator is an invalid state and can no longer make progress"); },
                [](Uninitialized) -> State { throw Exception::internal("Attempt to make progress on a uninitialized initiator."); },
                [&](Idle state) -> State { return state; },
                [&](WaitForAddrResolved state) -> State
                {
                    MXL_INFO("Check if address resolved");
                    if (auto event = readEventQueue<queueReadMode>(state.cm, timeout);
                        event && event.value().isSuccess() && event.value().isAddrResolved())
                    {
                        MXL_INFO("Address Resolved!");
                        state.cm.resolveRoute(std::chrono::seconds(15));
                        MXL_INFO("Switching to state WaitForRouteResolved");
                        return WaitForRouteResolved{.cm = std::move(state.cm), .regions = state.regions};
                    }
                    return state;
                },
                [&](WaitForRouteResolved state) -> State
                {
                    if (auto event = readEventQueue<queueReadMode>(state.cm, timeout);
                        event && event.value().isSuccess() && event.value().isRouteResolved())
                    {
                        MXL_INFO("Route Resolved!");
                        state.cm.createCompletionQueue();
                        state.cm.createQueuePair(QueuePairAttr::defaults());

                        state.cm.connect();
                        return WaitConnection{.cm = std::move(state.cm), .regions = state.regions};
                    }
                    return state;
                },
                [&](WaitConnection state) -> State
                {
                    if (auto event = readEventQueue<queueReadMode>(state.cm, timeout);
                        event && event.value().isSuccess() && event.value().isConnectionEstablished())
                    {
                        MXL_INFO("Connected!");
                        return Connected{.cm = std::move(state.cm), .regions = std::move(state.regions)};
                    }
                    return state;
                },
                [&](Connected state) -> State
                {
                    if (auto event = readEventQueue<QueueReadMode::NonBlocking>(state.cm, timeout); event && event.value().isSuccess())
                    {
                        if (event.value().isDisconnected())
                        {
                            return Done{std::move(state.cm)};
                        }
                    }

                    if (auto comp = readCompletionQueue<queueReadMode>(state.cm, timeout); comp)
                    {
                        if (comp.value().isErr())
                        {
                            MXL_ERROR("CQ Error: {}", comp.value().errToString());
                        }
                        else
                        {
                            _pendingTransfer--;
                        }
                    }

                    status = _pendingTransfer > 0;

                    return state;
                },
                [](Done) -> State
                {
                    throw Exception::interrupted("Initiator Done!");
                }},
            std::move(_state));

        return status;
    }

    bool RCInitiator::makeProgress()
    {
        return makeProgressInternal<QueueReadMode::NonBlocking>({});
    }

    bool RCInitiator::makeProgressBlocking(std::chrono::steady_clock::duration timeout)

    {
        return makeProgressInternal<QueueReadMode::Blocking>(timeout);
    }

    RCInitiator::RCInitiator(State state, std::vector<LocalRegionGroup> localRegions)
        : _localRegions(std::move(localRegions))
        , _state(std::move(state))
    {}
}
