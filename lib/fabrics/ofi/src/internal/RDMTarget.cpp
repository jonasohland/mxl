#include "RDMTarget.hpp"
#include <memory>
#include <rdma/fabric.h>
#include "internal/Logging.hpp"
#include "AddressVector.hpp"
#include "Exception.hpp"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types

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

        auto fabricInfoList = FIInfoList::get(
            config.endpointAddress.node, config.endpointAddress.service, provider.value(), FI_RMA | FI_REMOTE_WRITE, FI_EP_RDM);

        // TODO:: this is copy-pasta... share this part with RCTarget!
        auto info = *fabricInfoList.begin();
        MXL_INFO("{}", fi_tostr(info.raw(), FI_TYPE_INFO));

        auto fabric = Fabric::open(info);
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

        auto endpoint = std::make_unique<Endpoint>(Endpoint::create(domain));

        MXL_INFO("Create CQ and bind");
        auto cq = CompletionQueue::open(domain, CompletionQueueAttr::defaults());
        endpoint->bind(cq, FI_RECV | FI_TRANSMIT);

        MXL_INFO("Create AV and bind");
        // Connectionless endpoints must be bound to an address vector. Even if it is not using the address vector.
        auto av = AddressVector::open(domain);
        endpoint->bind(av);

        MXL_INFO("Try to enable the ep");
        endpoint->enable();

        MXL_INFO("Enabled endpoint!");

        auto localAddress = endpoint->localAddress();
        MXL_INFO("Got fabricAddress");
        auto remoteRegions = toRemote(localRegions);

        for (auto const& group : remoteRegions)
        {
            for (auto const& region : group.view())
            {
                MXL_INFO("Remote region -> addr={:x} len={} rkey={:x}", region.addr, region.len, region.rkey);
            }
        }

        struct MakeUniqueEnabler : RDMTarget
        {
            MakeUniqueEnabler(std::unique_ptr<Endpoint> endpoint, std::vector<RegisteredRegionGroup> regions)
                : RDMTarget(std::move(endpoint), std::move(regions))
            {}
        };

        auto rdmTarget = std::make_unique<MakeUniqueEnabler>(std::move(endpoint), std::move(localRegions));
        MXL_INFO("Created rdm target!");

        auto targetInfo = std::make_unique<TargetInfo>(std::move(localAddress), std::move(remoteRegions));
        MXL_INFO("Created targetInfo");

        return {std::move(rdmTarget), std::move(targetInfo)};
    }

    Target::ReadResult RDMTarget::read()
    {
        return makeProgress();
    }

    Target::ReadResult RDMTarget::readBlocking(std::chrono::steady_clock::duration timeout)
    {
        return makeProgressBlocking(timeout);
    }

    RDMTarget::RDMTarget(std::unique_ptr<Endpoint> endpoint, std::vector<RegisteredRegionGroup> regions)
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

        auto [completion, _] = _endpoint->readQueuesBlocking(timeout);
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
