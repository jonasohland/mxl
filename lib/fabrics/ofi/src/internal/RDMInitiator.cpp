#include "RDMInitiator.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <rdma/fabric.h>
#include "internal/Logging.hpp"
#include "mxl/mxl.h"
#include "Address.hpp"
#include "AddressVector.hpp"
#include "CompletionQueue.hpp"
#include "Endpoint.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Provider.hpp"
#include "RegisteredRegion.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    RDMInitiatorEndpoint::RDMInitiatorEndpoint(std::shared_ptr<Endpoint> ep, FabricAddress remote, std::vector<RemoteRegionGroup> remoteRegions)
        : _state(Idle{})
        , _ep(std::move(ep))
        , _addr(std::move(remote))
        , _regions(std::move(remoteRegions))
    {}

    bool RDMInitiatorEndpoint::isIdle() const noexcept
    {
        return std::holds_alternative<Idle>(_state);
    }

    bool RDMInitiatorEndpoint::canEvict() const noexcept
    {
        return std::holds_alternative<Done>(_state);
    }

    bool RDMInitiatorEndpoint::hasPendingWork() const noexcept
    {
        return std::visit(
            overloaded{
                [](std::monostate) { return false; },                 // Something went wrong with this target, but there is probably no work to do.
                [](Idle const&) { return true; },                     // An idle target means there is no work to do right now.
                [](Added const& state) { return state.pending > 0; }, // In the added state, the target is waiting for a connected event.
                [](Done const&) { return false; },                    // In the done state, there is no pending work.
            },
            _state);
    }

    void RDMInitiatorEndpoint::shutdown()
    {
        _state = std::visit(
            overloaded{
                [](Idle) -> State
                {
                    MXL_INFO("Shutdown requested while waiting to activate, aborting.");
                    return Done{};
                },
                [&](Added state) -> State
                {
                    MXL_INFO("Shutting down");
                    try
                    {
                        _ep->addressVector()->remove(state.fiAddr);
                        return Done{};
                    }
                    catch (...)
                    {}
                    return state;
                },
                [](Done state) -> State { return state; },
            },
            std::move(_state));
    }

    void RDMInitiatorEndpoint::activate()
    {
        _state = std::visit(
            overloaded{
                [&](Idle state) -> State
                {
                    try
                    {
                        MXL_INFO("Attempt to insert an address to the AV");

                        auto fiAddr = _ep->addressVector()->insert(_addr);
                        MXL_INFO("Switching state to 'Added' with fiAddr={}", fiAddr);
                        return Added{.fiAddr = fiAddr, .pending = 0};
                    }
                    catch (...)
                    {
                        MXL_ERROR("got a throw while activating !");
                    }

                    return state;
                },
                [](Added state) -> State { return state; },
                [](Done state) -> State { return state; },
            },
            std::move(_state));
    }

    void RDMInitiatorEndpoint::consume(Completion completion)
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

    void RDMInitiatorEndpoint::postTransfer(LocalRegionGroup const& local, uint64_t index)
    {
        if (auto state = std::get_if<Added>(&_state); state != nullptr)
        {
            MXL_INFO("Grain Index {} Remote Region Size = {}", index, _regions.size());

            // Find the remote region to which this grain should be written.
            auto const& remote = _regions[index % _regions.size()];

            // Post a write work item to the endpoint and increment the pending counter. When the write is complete, a completion will be posted to
            // the completion queue, after which the counter will be decremented again if the target is still present
            _ep->write(local, remote, state->fiAddr, index);
            MXL_INFO("POSTED REMOTE WRITE");
            ++state->pending;
        }
    }

    void RDMInitiatorEndpoint::handleCompletionData(Completion::Data)
    {
        _state = std::visit(
            overloaded{[](Idle state) -> State
                {
                    MXL_WARN("Received a completion event while idle, ignoring.");
                    return state;
                },
                [](Added state) -> State
                {
                    if (state.pending == 0)
                    {
                        MXL_WARN("Received a completion but no transfer was pending");
                        return state;
                    }

                    --state.pending;

                    return state;
                },
                [](Done state) -> State
                {
                    MXL_DEBUG("Ignoring completion after shutdown");
                    return state;
                }},
            std::move(_state));
    }

    void RDMInitiatorEndpoint::handleCompletionError(Completion::Error err)
    {
        MXL_ERROR("TODO: handle completion error: {}", err.toString());
    }

    std::unique_ptr<RDMInitiator> RDMInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No provider available.");
        }

        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_WRITE | FI_REMOTE_WRITE, FI_EP_RDM);

        auto info = (*fabricInfoList.begin()).owned();
        MXL_INFO("{}", fi_tostr(info.raw(), FI_TYPE_INFO));

        auto fabric = Fabric::open(info.view());
        auto domain = Domain::open(fabric);

        auto endpoint = std::make_shared<Endpoint>(Endpoint::create(domain));

        MXL_INFO("Open CQ");
        auto cq = CompletionQueue::open(domain);
        endpoint->bind(cq, FI_TRANSMIT | FI_RECV);

        MXL_INFO("Open AV");
        auto av = AddressVector::open(domain);
        endpoint->bind(av);

        // TODO : this logic is repeated in all initiator and target, DRY!
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

        // TODO:
        struct MakeUniqueEnabler : RDMInitiator
        {
            MakeUniqueEnabler(std::shared_ptr<Endpoint> ep, std::vector<RegisteredRegionGroup> regions)
                : RDMInitiator(std::move(ep), regions)
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(std::move(endpoint), std::move(registeredRegions));
    }

    void RDMInitiator::addTarget(TargetInfo const& targetInfo)
    {
        _targets.emplace(targetInfo.id, RDMInitiatorEndpoint(_endpoint, targetInfo.fabricAddress, targetInfo.remoteRegionGroups));
    }

    void RDMInitiator::removeTarget(TargetInfo const& targetInfo)
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

    void RDMInitiator::transferGrain(uint64_t index)

    {
        MXL_INFO("Grain Index = {} LocalRegion Size = {}", index, _localRegions.size());
        // Find the local region in which the grain with this index is stored.
        auto& localRegion = _localRegions[index % _localRegions.size()];

        // Post a transfer work item to all targets. If the target is not in a connected state
        // this is a no-op.
        for (auto& [_, target] : _targets)
        {
            target.postTransfer(localRegion, index);
        }
    }

    // makeProgress
    bool RDMInitiator::makeProgress()
    {
        consolidateState();
        pollCQ();
        return hasPendingWork();
    }

    // makeProgressBlocking
    bool RDMInitiator::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        consolidateState();
        blockOnCQ(timeout);
        return hasPendingWork();
    }

    RDMInitiator::RDMInitiator(std::shared_ptr<Endpoint> ep, std::vector<RegisteredRegionGroup> regions)
        : _endpoint(std::move(ep))
        , _registeredRegions(regions)
        , _localRegions(toLocal(regions))
    {}

    bool RDMInitiator::hasPendingWork() const noexcept
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

    void RDMInitiator::blockOnCQ(std::chrono::system_clock::duration timeout)
    {
        MXL_INFO("Block on CQ");
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
            auto completion = _endpoint->completionQueue()->readBlocking(timeout);
            if (!completion)
            {
                return;
            }

            // Find the endpoint that this completion was generated from
            // auto remoteEp = _targets.find(Endpoint::idFromFID(completion->fid()));
            // if (remoteEp == _targets.end())
            // {
            //     MXL_WARN("Received completion for an unknown endpoint");
            // }

            // return remoteEp->second.consume(*completion);

            MXL_INFO("received completion '");
            return;
        }
    }

    void RDMInitiator::pollCQ()
    {
        MXL_INFO("Polling CQ");
        for (;;)
        {
            auto completion = _endpoint->completionQueue()->read();
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

    void RDMInitiator::activateIdleEndpoints()
    {
        for (auto& [_, target] : _targets)
        {
            target.activate();
        }
    }

    void RDMInitiator::evictDeadEndpoints()
    {
        std::erase_if(_targets, [](auto const& item) { return item.second.canEvict(); });
    }

    void RDMInitiator::consolidateState()
    {
        activateIdleEndpoints();
        evictDeadEndpoints();
    }

} // namespace mxl::lib::fabrics::ofi
