#include "Target.hpp"
#include <chrono>
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "EventQueue.hpp"
#include "EventQueueEntry.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types
#include "PassiveEndpoint.hpp"
#include "RMATarget.hpp"

namespace mxl::lib::fabrics::ofi
{
    TargetWrapper::TargetWrapper()
    {}

    TargetWrapper::~TargetWrapper()
    {}

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config)
    {
        namespace ranges = std::ranges;

        // we're still missing a way to use the flow uuid and recover the addresses to register (MR)

        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}, flow = {}]",
            config.endpointAddress.node,
            config.endpointAddress.service,
            config.provider,
            config.flowId);

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service);

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(fabric);

        auto passiveEndpoint = PassiveEndpoint::create(fabric);

        auto pepEq = EventQueue::open(fabric, EventQueueAttr::get_default());
        passiveEndpoint->bind(*pepEq);

        passiveEndpoint->listen();

        // start connection event loop
        for (;;)
        {
            // wait for connection request
            auto entry = pepEq->waitForEntry(std::chrono::milliseconds(200));
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
            endpoint->bind(*cq, FI_REMOTE_WRITE | FI_RECV);

            auto eq = EventQueue::open(fabric, EventQueueAttr::get_default());
            endpoint->bind(*eq);

            // we are now ready to accept the connection
            endpoint->accept();

            // wait for a connected event
            for (;;)
            {
                auto entry = eq->waitForEntry(std::chrono::milliseconds(200));
                if (!entry.has_value())
                {
                    continue;
                }

                if (entry.value()->getEventType() == ConnNotificationEntry::EventType::Connected)
                {
                    MXL_INFO("Connection established with endpoint [node = {}, service = {}]",
                        config.endpointAddress.node,
                        config.endpointAddress.service);
                    break;
                }
            }

            // we are now connected!!
        }

        return {MXL_STATUS_OK, std::make_unique<TargetInfo>()};
    }
}
