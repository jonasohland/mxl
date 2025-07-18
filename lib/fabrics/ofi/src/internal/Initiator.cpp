#include "Initiator.hpp"
#include <memory>
#include <vector>
#include <rdma/fabric.h>
#include "mxl/fabrics.h"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Provider.hpp"
#include "RMATarget.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::pair<mxlStatus, std::unique_ptr<Initiator>> Initiator::setup(mxlInitiatorConfig const& config)
    {
        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, providerFromAPI(config.provider));

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(fabric);

        // TODO: Normally we should register our local memory regions here (buffers we will initiate transfers with).

        struct MakeUniqueEnabler : public Initiator
        {
            MakeUniqueEnabler(std::shared_ptr<Domain> domain, std::vector<std::shared_ptr<Endpoint>> endpoints)
                : Initiator(std::move(domain), std::move(endpoints))
            {}
        };

        return {MXL_STATUS_OK, std::make_unique<MakeUniqueEnabler>(domain, std::vector<std::shared_ptr<Endpoint>>{})};
    }

    mxlStatus Initiator::addTarget(TargetInfo const& targetInfo)
    {
        auto endpoint = Endpoint::create(_domain);

        auto eq = EventQueue::open(_domain->fabric(), EventQueueAttr::get_default());
        endpoint->bind(eq);

        auto cq = CompletionQueue::open(_domain, CompletionQueueAttr::get_default());
        endpoint->bind(cq, FI_WRITE);

        // targetInfo.regions() // TODO: Keep those for later use when transferring data
        // targetInfo.rkey() // TODO: Keep those for later use when transferring data

        // connect to the target
        endpoint->connect(targetInfo.fabricAddress());

        // We should probably store the endpoint and target info in a map so we can retrieve it later
        _endpoints.push_back(std::move(endpoint));

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::removeTarget(TargetInfo const& targetInfo)
    {
        // Find endpoint associated with the targetInfo and remove it

        MXL_FABRICS_UNUSED(targetInfo);
        return MXL_STATUS_OK;
    }
}