#include "RDMTarget.hpp"
#include <memory>
#include <catch2/catch_tostring.hpp>
#include "internal/Logging.hpp"
#include "Exception.hpp"
#include "FIInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::pair<std::unique_ptr<RDMTarget>, std::unique_ptr<TargetInfo>> RDMTarget::setup(mxlTargetConfig const& config)
    {
        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}]", config.endpointAddress.node, config.endpointAddress.service, config.provider);

        auto provider = providerFromAPI(config.provider);
        if (!provider)
        {
            throw Exception::invalidArgument("Invalid provider specified");
        }

        // TODO:: this is copy-pasta... share this part with RCTarget!
        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_REMOTE_WRITE);

        auto info = fabricInfoList.begin();

        auto fabric = Fabric::open(*info);
        auto domain = Domain::open(fabric);

        std::vector<RegisteredRegionGroup> localRegions;

        // config.regions contains the local memory regions that the remote writer will write to.
        auto localRegionGroups = RegionGroups::fromAPI(config.regions);

        for (auto const& group : localRegionGroups->view())
        {
            // For each region group we will register memory and keep a copy inside this instance as a list of registered region groups.
            // Registered regions can be converted to remote region groups or local region groups where needed.
            std::vector<RegisteredRegion> registeredGroup;
            for (auto const& region : group.view())
            {
                registeredGroup.emplace_back(MemoryRegion::reg(domain, region, FI_REMOTE_WRITE), region);
            }

            localRegions.emplace_back(registeredGroup);
        }
        // TODO: share this part with RCTarget!

        auto endpoint = Endpoint::create(domain);

        auto cq = CompletionQueue::open(domain, CompletionQueueAttr::defaults());
        endpoint.bind(cq, FI_RECV);

        endpoint.enable();

        auto localAddress = endpoint.localAddress();
        auto remoteRegions = toRemote(localRegions);

        struct MakeUniqueEnabler : RDMTarget
        {
            MakeUniqueEnabler(Endpoint endpoint, std::vector<RegisteredRegionGroup> regions)
                : RDMTarget(std::move(endpoint), regions)
            {}
        };

        return {std::make_unique<MakeUniqueEnabler>(std::move(endpoint), std::move(localRegions)),
            std::make_unique<TargetInfo>(localAddress, std::move(remoteRegions))};
    }

    Target::ReadResult RDMTarget::read()
    {
        return makeProgress();
    }

    Target::ReadResult RDMTarget::readBlocking(std::chrono::steady_clock::duration timeout)
    {
        return makeProgressBlocking(timeout);
    }

    RDMTarget::RDMTarget(Endpoint endpoint, std::vector<RegisteredRegionGroup> regions)
        : _endpoint(std::move(endpoint))
        , _regRegions(std::move(regions))
    {}

    Target::ReadResult RDMTarget::makeProgress()
    {
        throw Exception::internal("Not implemented");
    }

    Target::ReadResult RDMTarget::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        Target::ReadResult result;

        auto [completion, _] = _endpoint.readQueuesBlocking(timeout);
        if (completion)
        {
            if (auto dataEntry = completion.value().tryData(); dataEntry)
            {
                // The written grain index is sent as immediate data, and was returned
                // from the completion queue.
                result.grainAvailable = dataEntry->data();
                // TODO: check if we need to post a recv request like for the RCTarget
            }
        }

        return result;
    }

}
