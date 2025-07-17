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
#include "Address.hpp"
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
#include "RMATarget.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    TargetWrapper::~TargetWrapper()
    {}

    void TargetWrapper::doProgress()
    {
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
                        // When creating the endpoint here must pass the info of the remote endpoint requesting connection
                        auto endpoint = Endpoint::create(*_domain, entry->get()->info());

                        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
                        endpoint->bind(cq, FI_REMOTE_WRITE | FI_RECV);

                        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
                        endpoint->bind(eq);

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
                    // TODO: handle grains!

                    return state;
                }},
            _state);
    }

    void TargetWrapper::doProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
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
                        // TODO: move this to a separate function to avoid code duplication
                        // create the active endpoint and bind a cq and an eq
                        auto endpoint = Endpoint::create(*_domain);

                        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
                        endpoint->bind(cq, FI_REMOTE_WRITE | FI_RECV);

                        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
                        endpoint->bind(eq);

                        // we are now ready to accept the connection
                        endpoint->accept();

                        // Return the new state as the variant type
                        return StateWaitForConnected{endpoint};
                    }

                    return state;
                },
                [&](StateWaitForConnected& state) -> State
                {
                    if (auto entry = state.ep->eventQueue()->waitForEntry(timeout); entry && entry->get()->isConnected())
                    {
                        // We have a connected event, so we can transition to the connected state
                        return StateConnected{state.ep};
                    }

                    return state;
                },
                [&](StateConnected& state) -> State
                {
                    // TODO: handle grains!

                    return state;
                }},
            _state);
    }

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config) noexcept
    {
        namespace ranges = std::ranges;
        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}]", config.endpointAddress.node, config.endpointAddress.service, config.provider);

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, providerFromAPI(config.provider));

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            return {MXL_ERR_NO_FABRIC, nullptr};
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        _domain = Domain::open(fabric);

        auto regions = Regions::fromAPI(config.regions);
        _mr = MemoryRegion::reg(*_domain, *regions, FI_REMOTE_WRITE);

        auto pep = PassiveEndpoint::create(fabric);

        auto eq = EventQueue::open(fabric, EventQueueAttr::get_default());
        pep->bind(eq);
        pep->listen();

        // Transition the state machine to the waiting for a connection request state
        _state = StateWaitConnReq{pep};

        return {MXL_STATUS_OK, std::make_unique<TargetInfo>(FabricAddress::fromEndpoint(*pep), *regions, _mr->get()->getRemoteKey())};
    }
}
