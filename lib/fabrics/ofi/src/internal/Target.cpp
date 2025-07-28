#include "Target.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <variant>
#include <fmt/format.h>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "EventQueue.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types
#include "MemoryRegion.hpp"
#include "PassiveEndpoint.hpp"
#include "Provider.hpp"
#include "Region.hpp"
#include "RegisteredRegion.hpp"
#include "RMATarget.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{

    TargetProgressResult TargetWrapper::doProgress()
    {
        TargetProgressResult result;

        _state = std::visit(
            overloaded{[](StateFresh& state) -> State
                {
                    // Nothing to do, we are waiting for a setup call
                    return state;
                },
                [&](StateWaitConnReq& state) -> State
                {
                    // Check if the entry is available and is a connection request
                    if (auto entry = state.pep->eventQueue()->tryEntry(); entry && entry->get()->isConnReq())
                    {
                        auto remoteInfo = std::make_shared<FIInfo>(entry->get()->info()->owned());

                        // When creating the endpoint here we must pass the info of the remote endpoint requesting connection
                        auto endpoint = Endpoint::create(*_domain, remoteInfo);

                        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
                        endpoint->bind(std::move(cq), FI_REMOTE_WRITE | FI_RECV);

                        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
                        endpoint->bind(std::move(eq));

                        // we are now ready to accept the connection
                        endpoint->accept();

                        // Return the new state as the variant type
                        return StateWaitForConnected{endpoint};
                    }

                    return state;
                },
                [](StateWaitForConnected& state) -> State
                {
                    auto eq = state.ep->eventQueue();

                    if (auto entry = eq->tryEntry(); entry && entry->get()->isConnected())
                    {
                        // We have a connected event, so we can transition to the connected state
                        return StateConnected{state.ep};
                    }

                    return state;
                },
                [&](StateConnected& state) -> State
                {
                    if (auto entry = state.ep->completionQueue()->tryEntry(); entry)
                    {
                        if (auto data = entry.value().data(); data)
                        {
                            result.grainCompleted = data.value();
                        }
                    }
                    return state;
                }},
            _state);

        return result;
    }

    TargetProgressResult TargetWrapper::doProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        TargetProgressResult result;

        _state = std::visit(
            overloaded{[](StateFresh& state) -> State
                {
                    // Nothing to do, we are waiting for a setup call
                    return state;
                },
                [&](StateWaitConnReq& state) -> State
                {
                    // Check if the entry is available and is a connection request
                    if (auto entry = state.pep->eventQueue()->waitForEntry(timeout); entry && entry->get()->isConnReq())
                    {
                        MXL_INFO("Connection request received, creating endpoint for remote address: {}", entry->get()->info().value()->dest_addr);

                        auto remoteInfo = std::make_shared<FIInfo>(entry.value()->info()->owned());

                        // TODO: move this to a separate function to avoid code duplication
                        // When creating the endpoint here we must pass the info of the remote endpoint requesting connection
                        auto endpoint = Endpoint::create(*_domain, remoteInfo);

                        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
                        endpoint->bind(cq, FI_RECV);

                        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
                        endpoint->bind(eq);

                        // we are now ready to accept the connection
                        endpoint->accept();
                        MXL_INFO("Accepted the connection waiting for connected event notification.");
                        // TODO: move this to a separate function to avoid code duplication

                        // Return the new state as the variant type
                        return StateWaitForConnected{endpoint};
                    }

                    return state;
                },
                [&](StateWaitForConnected& state) -> State
                {
                    if (auto entry = state.ep->eventQueue()->waitForEntry(timeout); entry && entry->get()->isConnected())
                    {
                        MXL_INFO("Received connected event notification, now connected.");

                        // We have a connected event, so we can transition to the connected state
                        return StateConnected{state.ep};
                    }

                    return state;
                },
                [&](StateConnected& state) -> State
                {
                    if (auto entry = state.ep->completionQueue()->waitForEntry(timeout); entry)
                    {
                        if (auto data = entry.value().data(); data)
                        {
                            result.grainCompleted = data.value();
                        }
                    }

                    return state;
                }},
            _state);

        return result;
    }

    TargetWrapper* TargetWrapper::fromAPI(mxlFabricsTarget api) noexcept
    {
        if (api == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<TargetWrapper*>(api);
    }

    mxlFabricsTarget TargetWrapper::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsTarget>(this);
    }

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config) noexcept
    {
        namespace ranges = std::ranges;
        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}]", config.endpointAddress.node, config.endpointAddress.service, config.provider);

        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            return {MXL_ERR_INVALID_ARG, nullptr};
        }

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, provider.value());

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            return {MXL_ERR_NO_FABRIC, nullptr};
        }

        auto fabricInfo = std::make_shared<FIInfo>(bestFabricInfo->owned());

        auto fabric = Fabric::open(fabricInfo);
        _domain = Domain::open(fabric);

        // config.regions contains the memory layout that will be written into.
        auto* regions = RegionGroups::fromAPI(config.regions);

        // auto group = regions->view().front();
        for (auto const& group : regions->view())
        {
            // For each region group we will register memory and keep a copy inside this instance as a list of registered region groups.
            // Registered regions can be converted to remote region groups or local region groups where needed.
            std::vector<RegisteredRegion> registeredGroup;
            for (auto const& region : group.view())
            {
                auto mr = MemoryRegion::reg(*_domain, region, FI_REMOTE_WRITE);
                auto registeredRegion = RegisteredRegion(std::move(mr), region);
                registeredGroup.emplace_back(std::move(registeredRegion));
            }
            _regRegions.emplace_back(registeredGroup);
        }

        auto pep = PassiveEndpoint::create(fabric);

        auto eq = EventQueue::open(fabric, EventQueueAttr::get_default());
        pep->bind(eq);
        pep->listen();

        // TODO: delete me
        for (auto const& group : toRemote(_regRegions))
        {
            for (auto const& region : group.view())
            {
                MXL_INFO("Remote region addr={:x} len={} rkey={:x}", region.addr, region.len, region.rkey);
            }
        }
        // TODO: delete me

        // Transition the state machine to the waiting for a connection request state
        MXL_INFO("Listening for initator connection request.");
        _state = StateWaitConnReq{pep};

        return {MXL_STATUS_OK, std::make_unique<TargetInfo>(pep->localAddress(), toRemote(_regRegions))};
    }

    TargetProgressResult TargetWrapper::tryGrain()
    {
        return doProgress();
    }

    TargetProgressResult TargetWrapper::waitForGrain(std::chrono::steady_clock::duration timeout)
    {
        return doProgressBlocking(timeout);
    }
}
