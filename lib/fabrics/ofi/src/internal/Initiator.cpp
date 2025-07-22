#include "Initiator.hpp"
#include <cstdint>
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
        if (!regions)
        {
            return MXL_ERR_UNKNOWN; // TODO: proper error
        }

        for (auto const& region : *regions)
        {
            auto mr = MemoryRegion::reg(*_domain, region, FI_WRITE);
            _localRegions.emplace_back(std::move(mr), region);
        }

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::addTarget(TargetInfo const& targetInfo)
    {
        auto endpoint = Endpoint::create(*_domain);

        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
        endpoint->bind(eq);

        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
        endpoint->bind(cq, FI_WRITE);

        endpoint->connect(targetInfo.fabricAddress);

        auto target = InitiatorTargetEntry{.endpoint = std::move(endpoint), .regions = targetInfo.remoteRegions};

        _targets.insert({targetInfo.asIdentifier(), target});

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::removeTarget(TargetInfo const& targetInfo)
    {
        MXL_FABRICS_UNUSED(targetInfo);

        auto identifier = targetInfo.asIdentifier();

        if (_targets.contains(identifier))
        {
            auto target = _targets.at(identifier);

            target.endpoint->shutdown();

            _targets.erase(identifier);

            return MXL_STATUS_OK;
        }

        return MXL_ERR_FLOW_NOT_FOUND; // TODO: replace this with a more proper error code
    }

    mxlStatus Initiator::transferGrain(uint64_t grainIndex, GrainInfo const* grainInfo, uint8_t const* payload)
    {
        if (grainInfo == nullptr || payload == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        auto localRegion = _localRegions.at(grainIndex % _localRegions.size());

        for (auto [ident, target] : _targets)
        {
            auto remoteRegion = target.regions.at(grainIndex % target.regions.size());

            target.endpoint->write(localRegion.region(), localRegion.desc(), remoteRegion.addr, remoteRegion.rkey);
        }

        return MXL_STATUS_OK;
    }

}
