#include "Initiator.hpp"
#include <cstdint>
#include <memory>
#include <uuid.h>
#include <rdma/fabric.h>
#include <rfl/json/write.hpp>
#include "internal/Logging.hpp"
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

    namespace internal
    {
        mxlStatus removeTargetUnchecked(uuids::uuid const& id, std::map<uuids::uuid, InitiatorTargetEntry>& targets)
        {
            auto target = targets.at(id);

            target.endpoint->shutdown();

            targets.erase(id);

            return MXL_STATUS_OK;
        }
    }

    Initiator::~Initiator()
    {
        for (auto [ident, target] : _targets)
        {
            internal::removeTargetUnchecked(ident, _targets);
        }
    }

    Initiator* Initiator::fromAPI(mxlFabricsInitiator api)
    {
        if (api == nullptr)
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "Invalid fabrics initiator API pointer");
        }

        return reinterpret_cast<Initiator*>(api);
    }

    mxlFabricsInitiator Initiator::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsInitiator>(this);
    }

    mxlStatus Initiator::setup(mxlInitiatorConfig const& config)
    {
        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            return MXL_ERR_INVALID_ARG;
        }

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, provider.value());

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabricInfo = std::make_shared<FIInfo>(bestFabricInfo->owned());

        auto fabric = Fabric::open(fabricInfo);
        _domain = Domain::open(fabric);

        auto regionGroups = RegionGroups::fromAPI(config.regions);
        if (!regionGroups)
        {
            return MXL_ERR_UNKNOWN;
        }

        MXL_INFO("------- Local Regions ---------");
        regionGroups->print();

        for (auto const& group : regionGroups->view())
        {
            std::vector<RegisteredRegion> regRegions;
            for (auto const& region : group.view())
            {
                auto mr = MemoryRegion::reg(*_domain, region, FI_WRITE);
                auto registeredRegion = RegisteredRegion(std::move(mr), region);
                regRegions.emplace_back(std::move(registeredRegion));
            }

            RegisteredRegionGroup regGroup{std::move(regRegions)};
            _localRegions.emplace_back(regGroup.toLocal());
            _registeredRegions.emplace_back(std::move(regGroup));
        }

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::addTarget(TargetInfo const& targetInfo)
    {
        if (_targets.contains(targetInfo.identifier))
        {
            // Target already exists!
            return MXL_ERR_INVALID_ARG;
        }

        auto endpoint = Endpoint::create(*_domain, _domain->get()->fabric()->info());

        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
        endpoint->bind(eq);

        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
        endpoint->bind(cq, FI_TRANSMIT);

        MXL_INFO("Connecting endpoint to target with address: {}", rfl::json::write(targetInfo.fabricAddress));
        endpoint->connect(targetInfo.fabricAddress);

        while (true)
        {
            auto entry = eq->waitForEntry(std::chrono::seconds(1));
            if (entry && entry.value()->isConnected())
            {
                break;
            }
        }
        MXL_INFO("Now connected!");

        auto target = InitiatorTargetEntry{.endpoint = std::move(endpoint), .remoteGroups = targetInfo.remoteRegionGroups};

        MXL_INFO("-------- Remote Regions ------------");
        for (auto const& group : target.remoteGroups)
        {
            for (auto const& region : group.view())
            {
                MXL_INFO("Remote region addr={:x} len={} rkey={:x}", region.addr, region.len, region.rkey);
            }
        }

        _targets.insert({targetInfo.identifier, target});

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::removeTarget(TargetInfo const& targetInfo)
    {
        auto identifier = targetInfo.identifier;

        if (_targets.contains(identifier))
        {
            return internal::removeTargetUnchecked(identifier, _targets);
        }

        // Can't remove a target that is not present!
        return MXL_ERR_INVALID_ARG;
    }

    mxlStatus Initiator::transferGrain(uint64_t grainIndex)
    {
        auto localOffset = grainIndex % _registeredRegions.size();
        MXL_INFO("TransferGrain -> localSize={}    localOffset={}", _registeredRegions.size(), localOffset);

        for (auto [ident, target] : _targets)
        {
            auto remoteOffset = grainIndex % target.remoteGroups.size();

            MXL_INFO("TransferGrain -> remoteSize={} remoteOffset={}", target.remoteGroups.size(), remoteOffset);
            target.endpoint->write(_localRegions.at(localOffset), target.remoteGroups.at(remoteOffset), FI_ADDR_UNSPEC, grainIndex);

            target.endpoint->completionQueue()->tryEntry();
        }

        return MXL_STATUS_OK;
    }
}
