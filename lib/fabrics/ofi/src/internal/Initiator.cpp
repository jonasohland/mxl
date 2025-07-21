#include "Initiator.hpp"
#include <memory>
#include <rdma/fabric.h>
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "MemoryRegion.hpp"
#include "Provider.hpp"
#include "RMATarget.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    mxlStatus Initiator::setup(mxlInitiatorConfig const& config)
    {
        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, providerFromAPI(config.provider));

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        _domain = Domain::open(fabric);

        auto regions = Regions::fromAPI(config.regions);
        _mr = MemoryRegion::reg(*_domain, *regions, FI_WRITE);

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::addTarget(std::string identifier, TargetInfo const& targetInfo)
    {
        auto endpoint = Endpoint::create(*_domain);

        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
        endpoint->bind(eq);

        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
        endpoint->bind(cq, FI_WRITE);

        endpoint->connect(targetInfo.fabricAddress());

        auto target = InitiatorTargetEntry{._endpoint = std::move(endpoint), ._regions = targetInfo.regions(), ._rkey = targetInfo.rkey()};

        _targets.insert({identifier, target});

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::removeTarget(std::string identifier, TargetInfo const& targetInfo)
    {
        // TODO: should the identifier just be BASE64 encoding of TargetInfo ?
        if (_targets.contains(identifier))
        {
            auto target = _targets.at(identifier);

            target._endpoint->shutdown();

            _targets.erase(identifier);

            return MXL_STATUS_OK;
        }

        return MXL_ERR_FLOW_NOT_FOUND; // TODO: replace this with a more proper error code
    }

    mxlStatus Initiator::transferGrainToTarget(std::string identifier, uint64_t grainIndex, GrainInfo const* grainInfo, uint8_t const* payload)
    {
        if (grainInfo == nullptr || payload == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        if (!_targets.contains(identifier))
        {
            return MXL_ERR_INVALID_ARG;
        }

        auto& target = _targets.at(identifier);

        auto r = target._regions;

        auto bufferEntryOffset = grainIndex % bufferLen;

        return MXL_STATUS_OK;
    }

}
