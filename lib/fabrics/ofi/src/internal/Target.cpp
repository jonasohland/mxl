#include "Target.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <fmt/format.h>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <sys/types.h>
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
#include "LocalRegion.hpp"
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
                [&](StateWaitForConnected& state) -> State
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
                        if (auto dataEntry = entry.value().tryData(); dataEntry)
                        {
                            MXL_INFO("Got an entry!");
                            auto localRegion = LocalRegion{
                                .addr = reinterpret_cast<std::uintptr_t>(&_immData), .len = sizeof(_immData), .desc = nullptr};
                            state.ep->recv(localRegion);

                            result.grainCompleted = dataEntry.value().data();
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
                        MXL_DEBUG("Connection request received, creating endpoint for remote address: {}", entry->get()->info().value()->dest_addr);

                        auto remoteInfo = std::make_shared<FIInfo>(entry.value()->info()->owned());
                        auto endpoint = Endpoint::create(*_domain, remoteInfo);

                        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
                        endpoint->bind(cq, FI_RECV);

                        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
                        endpoint->bind(eq);

                        // we are now ready to accept the connection
                        endpoint->accept();
                        MXL_DEBUG("Accepted the connection waiting for connected event notification.");

                        // Return the new state as the variant type
                        return StateWaitForConnected{endpoint};
                    }

                    return state;
                },
                [&](StateWaitForConnected& state) -> State
                {
                    if (auto entry = state.ep->eventQueue()->waitForEntry(timeout); entry && entry->get()->isConnected())
                    {
                        state.ep->recv(_immDataRegion);

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
                        if (auto dataEntry = entry.value().tryData(); dataEntry)
                        {
                            result.grainCompleted = dataEntry.value().data();

                            state.ep->recv(_immDataRegion);
                        }
                        else
                        {
                            MXL_ERROR("CQ Error={}", entry->err().toString(state.ep->completionQueue()->raw()));
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
        MXL_DEBUG(
            "setting up target [endpoint = {}:{}, provider = {}]", config.endpointAddress.node, config.endpointAddress.service, config.provider);

        auto provider = Provider::fromAPI(config.provider);
        if (!provider)
        {
            return {MXL_ERR_INVALID_ARG, nullptr};
        }

        uint64_t caps = FI_RMA | FI_REMOTE_WRITE;
        caps |= config.deviceSupport ? FI_HMEM : 0;

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, provider.value(), caps);

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            return {MXL_ERR_NO_FABRIC, nullptr};
        }

        auto fabricInfo = std::make_shared<FIInfo>(bestFabricInfo->owned());

        MXL_INFO("Selected provider: {}", ::fi_tostr(fabricInfo->raw(), FI_TYPE_INFO));

        auto fabric = Fabric::open(fabricInfo);

        _domain = Domain::open(fabric);
        if (_domain.value()->usingRecvBufForCqData())
        {
            _immDataRegion = LocalRegion{.addr = reinterpret_cast<std::uintptr_t>(_immData), .len = sizeof(_immData), .desc = nullptr};
        }

        // config.regions contains the memory layout that will be written into.
        auto* localRegionGroups = RegionGroups::fromAPI(config.regions);

        for (auto const& group : localRegionGroups->view())
        {
            // For each region group we will register memory and keep a copy inside this instance as a list of registered region groups.
            // Registered regions can be converted to remote region groups or local region groups where needed.
            std::vector<RegisteredRegion> registeredGroup;
            for (auto const& region : group.view())
            {
                registeredGroup.emplace_back(MemoryRegion::reg(*_domain, region, FI_WRITE | FI_REMOTE_READ | FI_READ | FI_REMOTE_WRITE), region);
            }
            _regRegions.emplace_back(registeredGroup);
        }

        auto pep = PassiveEndpoint::create(fabric);

        auto eq = EventQueue::open(fabric, EventQueueAttr::get_default());
        pep->bind(eq);
        pep->listen();

        // Transition the state machine to the waiting for a connection request state
        MXL_DEBUG("Listening for initator connection request.");
        _state = StateWaitConnReq{pep};

        auto remoteGroups = toRemote(_regRegions);
        for (auto const& group : remoteGroups)
        {
            for (auto const& region : group.view())
            {
                MXL_INFO("addr=0x{:x} len={} rkey=0x{:x}", region.addr, region.len, region.rkey);
            }
        }

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
