#include "Initiator.hpp"
#include <cstdint>
#include <memory>
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
        mxlStatus removeTargetUnchecked(std::string identifier, std::map<std::string, InitiatorTargetEntry>& targets)
        {
            auto target = targets.at(identifier);

            target.endpoint->shutdown();

            targets.erase(identifier);

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

        auto fabric = Fabric::open(*bestFabricInfo);
        _domain = Domain::open(fabric);

        auto regions = Regions::fromAPI(config.regions);
        if (!regions)
        {
            return MXL_ERR_UNKNOWN;
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
        if (_targets.contains(targetInfo.identifier()))
        {
            // Target already exists!
            return MXL_ERR_INVALID_ARG;
        }

        auto endpoint = Endpoint::create(*_domain);

        auto eq = EventQueue::open(_domain->get()->fabric(), EventQueueAttr::get_default());
        endpoint->bind(eq);

        auto cq = CompletionQueue::open(*_domain, CompletionQueueAttr::get_default());
        endpoint->bind(cq, 0);

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

        auto target = InitiatorTargetEntry{.endpoint = std::move(endpoint), .regions = targetInfo.regions};

        _targets.insert({targetInfo.identifier(), target});

        return MXL_STATUS_OK;
    }

    mxlStatus Initiator::removeTarget(TargetInfo const& targetInfo)
    {
        auto identifier = targetInfo.identifier();

        if (_targets.contains(identifier))
        {
            return internal::removeTargetUnchecked(identifier, _targets);
        }

        // Can't remove a target that is not present!
        return MXL_ERR_INVALID_ARG;
    }

    mxlStatus Initiator::transferGrain(uint64_t grainIndex)
    {
        auto localRegion = _localRegions.at(grainIndex % _localRegions.size());

        for (auto [ident, target] : _targets)
        {
            auto remoteRegion = target.regions.at(grainIndex % target.regions.size());

            target.endpoint->write(localRegion, remoteRegion);
        }

        return MXL_STATUS_OK;
    }
}
