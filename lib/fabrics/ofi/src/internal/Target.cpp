#include "Target.hpp"
#include <chrono>
#include <memory>
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
#include "EventQueueEntry.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types
#include "MemoryRegion.hpp"
#include "PassiveEndpoint.hpp"
#include "Provider.hpp"
#include "RMATarget.hpp"

namespace mxl::lib::fabrics::ofi
{
    TargetWrapper::TargetWrapper() = default;

    TargetWrapper::~TargetWrapper()
    {}

    std::shared_ptr<Endpoint> connectionEventLoop(PassiveEndpoint& pep, std::shared_ptr<Domain> domain)
    {
        pep.listen();

        // start connection event loop
        for (;;) // we probably need to use a cancellation token here, so that it doesn't run indefinitely
        {
            // wait for connection request
            auto entry = pep.eventQueue()->waitForEntry(std::chrono::milliseconds(200));
            if (!entry.has_value())
            {
                continue;
            }

            if (entry.value()->getEventType() != ConnNotificationEntry::EventType::ConnReq)
            {
                continue;
            }

            // create the active endpoint and bind a cq and an eq
            auto endpoint = Endpoint::create(domain);

            auto cq = CompletionQueue::open(domain, CompletionQueueAttr::get_default());
            endpoint->bind(cq, FI_REMOTE_WRITE | FI_RECV);

            auto eq = EventQueue::open(domain->fabric(), EventQueueAttr::get_default());
            endpoint->bind(eq);

            // we are now ready to accept the connection
            endpoint->accept();

            // wait for a connected event
            for (;;) // we probably need to use a cancellation token here, so that it doesn't run indefinitely
            {
                auto entry = eq->waitForEntry(std::chrono::milliseconds(200));
                if (!entry.has_value())
                {
                    continue;
                }

                if (entry.value()->getEventType() == ConnNotificationEntry::EventType::Connected)
                {
                    return endpoint;
                }
            }
        }
    }

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config)
    {
        namespace ranges = std::ranges;

        // we're still missing a way to use the flow uuid and recover the addresses to register (MR)

        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}, flow = {}]",
            config.endpointAddress.node,
            config.endpointAddress.service,
            config.provider,
            config.flowId);

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, providerFromAPI(config.provider));

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(fabric);

        auto pep = PassiveEndpoint::create(fabric);

        auto eq = EventQueue::open(fabric, EventQueueAttr::get_default());
        pep->bind(eq);

        auto ep = connectionEventLoop(*pep, domain);

        // we are now connected!!
        MXL_INFO("We are now connected to the endpoint!");

        auto fabricAddr = FabricAddress::fromEndpoint(*pep);
        auto regions = Regions(std::vector<Region>{});

        // TODO: correctly set the regions and rkey
        return {MXL_STATUS_OK, std::make_unique<TargetInfo>(*fabricAddr, regions, 0)};
    }
}
