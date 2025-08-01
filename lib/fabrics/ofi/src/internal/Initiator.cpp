#include "Initiator.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <uuid.h>
#include <rdma/fabric.h>
#include <rfl/json/write.hpp>
#include <sys/mman.h>
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
#include "Region.hpp"
#include "RegisteredRegion.hpp"
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

        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_WRITE | FI_REMOTE_WRITE);

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto info = bestFabricInfo->owned();

        MXL_INFO("Selected provider: {}", ::fi_tostr(info.raw(), FI_TYPE_INFO));

        auto fabricInfo = std::make_shared<FIInfo>(info);

        auto fabric = Fabric::open(fabricInfo);
        _domain = Domain::open(fabric);

        auto regionGroups = RegionGroups::fromAPI(config.regions);
        if (!regionGroups)
        {
            return MXL_ERR_UNKNOWN;
        }

        for (auto const& group : regionGroups->view())
        {
            std::vector<RegisteredRegion> regRegions;
            for (auto const& region : group.view())
            {
                regRegions.emplace_back(MemoryRegion::reg(*_domain, region, FI_WRITE), region);
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

        MXL_DEBUG("Connecting endpoint to target with encoded address: {}", rfl::json::write(targetInfo.fabricAddress));
        endpoint->connect(targetInfo.fabricAddress);

        // TODO: use a cancellation token to exit this loop
        for (;;)
        {
            auto entry = eq->waitForEntry(std::chrono::milliseconds(200));
            if (entry && entry.value()->isConnected())
            {
                break;
            }
        }
        MXL_INFO("Now connected!");

        _targets.insert({
            targetInfo.identifier, InitiatorTargetEntry{.endpoint = std::move(endpoint), .remoteGroups = targetInfo.remoteRegionGroups}
        });

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
        auto localOffset = grainIndex % _localRegions.size();

        for (auto [ident, target] : _targets)
        {
            // try to empty the completion queue
            flushCq(target);

            auto remoteOffset = grainIndex % target.remoteGroups.size();

            target.endpoint->write(_localRegions.at(localOffset), target.remoteGroups.at(remoteOffset), FI_ADDR_UNSPEC, grainIndex);
        }

        return MXL_STATUS_OK;
    }

    void Initiator::flushCq(InitiatorTargetEntry const& target) const noexcept
    {
        for (;;)
        {
            auto entry = target.endpoint->completionQueue()->tryEntry();
            if (!entry)
            {
                break;
            }

            if (auto error = entry->tryErr())
            {
                MXL_ERROR("Completion error entry available: {}", error->toString(target.endpoint->completionQueue()->raw()));
            }
        }
    }
}
