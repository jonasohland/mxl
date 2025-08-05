#include "RCInitiator.hpp"
#include <rfl/json/write.hpp>
#include "internal/Logging.hpp"
#include "Exception.hpp"
#include "RCTarget.hpp"

namespace mxl::lib::fabrics::ofi
{

    namespace internal
    {
        void removeTargetUnchecked(uuids::uuid const& id, std::map<uuids::uuid, InitiatorTargetEntry>& targets)
        {
            targets.at(id).endpoint.shutdown();
            targets.erase(id);
        }
    }

    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No provider available");
        }

        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_WRITE | FI_REMOTE_WRITE);

        auto bestFabricInfo = RCTarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto info = bestFabricInfo->owned();

        MXL_INFO("Selected provider: {}", ::fi_tostr(info.raw(), FI_TYPE_INFO));

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(fabric);

        auto regionGroups = RegionGroups::fromAPI(config.regions);

        std::vector<LocalRegionGroup> localRegions;
        std::vector<RegisteredRegionGroup> registeredRegions;

        for (auto const& group : regionGroups->view())
        {
            std::vector<RegisteredRegion> regRegions;
            for (auto const& region : group.view())
            {
                regRegions.emplace_back(MemoryRegion::reg(domain, region, FI_WRITE), region);
            }

            RegisteredRegionGroup regGroup{std::move(regRegions)};
            localRegions.emplace_back(regGroup.toLocal());
            registeredRegions.emplace_back(std::move(regGroup));
        }

        return nullptr;
    }

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        if (_targets.contains(targetInfo.identifier))
        {
            throw Exception::exists("Target id={} already exists for this initiator", uuids::to_string(targetInfo.identifier));
        }

        auto endpoint = Endpoint::create(*_domain, _domain->get()->fabric()->info());

        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::defaults());
        endpoint.bind(eq);

        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::defaults());
        endpoint.bind(cq, FI_TRANSMIT);

        MXL_DEBUG("Connecting endpoint to target with encoded address: {}", rfl::json::write(targetInfo.fabricAddress));
        endpoint.connect(targetInfo.fabricAddress);

        // TODO: use a cancellation token to exit this loop
        for (;;)
        {
            auto entry = eq->readEntryBlocking(std::chrono::milliseconds(200));
            if (entry && entry.value().isConnected())
            {
                break;
            }
        }

        MXL_INFO("Add target with {} groups", targetInfo.remoteRegionGroups.size());

        _targets.insert({
            targetInfo.identifier, InitiatorTargetEntry{.endpoint = std::move(endpoint), .remoteGroups = targetInfo.remoteRegionGroups}
        });
    }

    void RCInitiator::removeTarget(TargetInfo const& targetInfo)
    {
        auto identifier = targetInfo.identifier;

        if (_targets.contains(identifier))
        {
            internal::removeTargetUnchecked(identifier, _targets);
        }
    }

    void RCInitiator::transferGrain(uint64_t grainIndex)
    {
        auto localOffset = grainIndex % _localRegions.size();

        for (auto& [ident, target] : _targets)
        {
            // try to empty the completion queue
            // flushCq(target);

            auto remoteOffset = grainIndex % target.remoteGroups.size();

            target.endpoint.write(_localRegions.at(localOffset), target.remoteGroups.at(remoteOffset), FI_ADDR_UNSPEC, grainIndex);
        }
    }

}
