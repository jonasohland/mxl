#include "RCTarget.hpp"
#include <rdma/fabric.h>
#include "internal/Logging.hpp"
#include "mxl/mxl.h"
#include "Exception.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types
#include "Provider.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{

    RCTarget::RCTarget(std::shared_ptr<Domain> domain, std::vector<RegisteredRegionGroup> regions, PassiveEndpoint ep)
        : _domain(std::move(domain))
        , _regRegions(std::move(regions))
        , _state(WaitForConnectionRequest{std::move(ep)})
    {}

    Target::ReadResult RCTarget::read()
    {
        return makeProgress();
    }

    Target::ReadResult RCTarget::readBlocking(std::chrono::steady_clock::duration timeout)
    {
        return makeProgressBlocking(timeout);
    }

    std::pair<std::unique_ptr<RCTarget>, std::unique_ptr<TargetInfo>> RCTarget::setup(mxlTargetConfig const& config)
    {
        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}]", config.endpointAddress.node, config.endpointAddress.service, config.provider);

        // Convert to our internal enum type
        auto provider = Provider::fromAPI(config.provider);
        if (!provider)
        {
            throw Exception::invalidArgument("Invalid provider passed");
        }

        uint64_t caps = FI_RMA | FI_REMOTE_WRITE;
        caps |= config.deviceSupport ? FI_HMEM : 0;

        // Get a list of available fabric configurations available on this machine.
        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service, provider.value(), caps, FI_EP_MSG);

        if (fabricInfoList.begin() == fabricInfoList.end())
        {
            throw Exception::make(MXL_ERR_NO_FABRIC,
                "No fabric available for provider {} at {}:{}",
                config.provider,
                config.endpointAddress.node,
                config.endpointAddress.service);
        }

        // Open fabric and domain. These represent the context of the local network fabric adapter that will be used
        // to receive data.
        // See fi_domain(3) and fi_fabric(3) for more complete information about these concepts.
        auto fabric = Fabric::open(*fabricInfoList.begin());
        auto domain = Domain::open(fabric);

        // Create a passive endpoint. A passive endpoint can be viewed like a bound TCP socket listening for
        // incoming connections
        auto pep = PassiveEndpoint::create(fabric);

        // Create an event queue for the passive endpoint. Incoming connections generate an entry in the event queue
        // and be picked up when the Target tries to make progress.
        pep.bind(EventQueue::open(fabric, EventQueueAttr::defaults()));

        // Transition the PassiveEndpoint into a listening state. Connections will be accepted from now on.
        pep.listen();

        // Helper struct to enable the std::make_unique function to access the private constructor of this class
        struct MakeUniqueEnabler : RCTarget
        {
            MakeUniqueEnabler(std::shared_ptr<Domain> domain, std::vector<RegisteredRegionGroup> regions, PassiveEndpoint pep)
                : RCTarget(std::move(domain), std::move(regions), std::move(pep))
            {}
        };

        auto localRegisteredRegions = RegionGroups::fromAPI(config.regions)->reg(domain, FI_REMOTE_WRITE);
        auto remoteRegions = toRemote(localRegisteredRegions);

        auto localAddress = pep.localAddress();

        // Return the constructed RCTarget and associated TargetInfo for remote peers to connect.
        return {std::make_unique<MakeUniqueEnabler>(std::move(domain), std::move(localRegisteredRegions), std::move(pep)),
            std::make_unique<TargetInfo>(localAddress, std::move(remoteRegions))};
    }

    Target::ReadResult RCTarget::makeProgress()
    {
        throw Exception::internal("Not implemented");
    }

    Target::ReadResult RCTarget::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        Target::ReadResult result;

        _state = std::visit(
            overloaded{[](std::monostate) -> State { throw Exception::invalidState("Target is in an invalid state an can no longer make progress"); },
                [&](WaitForConnectionRequest state) -> State
                {
                    // Check if the entry is available and is a connection request
                    if (auto entry = state.pep.eventQueue()->readBlocking(timeout); entry && entry->isConnReq())
                    {
                        MXL_DEBUG("Connection request received, creating endpoint for remote address: {}", entry->info().raw()->dest_addr);
                        auto endpoint = Endpoint::create(_domain, entry->info());

                        auto cq = CompletionQueue::open(_domain, CompletionQueueAttr::defaults());
                        endpoint.bind(cq, FI_RECV);

                        auto eq = EventQueue::open(_domain->fabric(), EventQueueAttr::defaults());
                        endpoint.bind(eq);

                        // we are now ready to accept the connection
                        endpoint.accept();
                        MXL_DEBUG("Accepted the connection waiting for connected event notification.");

                        // Return the new state as the variant type
                        return WaitForConnection{std::move(endpoint)};
                    }

                    return state;
                },
                [&](WaitForConnection state) -> State
                {
                    if (auto entry = state.ep.eventQueue()->readBlocking(timeout); entry && entry->isConnected())
                    {
                        std::unique_ptr<ImmediateDataLocation> dataRegion;

                        // Need to post a receive buffer to get immediate data.
                        if (_domain->usingRecvBufForCqData())
                        {
                            // Create a local memory region. The grain indices will be written here when a transfer arrives.
                            dataRegion = std::make_unique<ImmediateDataLocation>();

                            // Post a receive for the first incoming grain. Pass a region to receive the grain index.
                            state.ep.recv(dataRegion->toLocalRegion());
                        }

                        MXL_INFO("Received connected event notification, now connected.");

                        // We have a connected event, so we can transition to the connected state
                        return Connected{.ep = std::move(state.ep), .immData = std::move(dataRegion)};
                    }

                    return state;
                },
                [&](Connected state) -> State
                {
                    auto [completion, event] = state.ep.readQueuesBlocking(timeout);
                    if (event && event.value().isShutdown())
                    {
                        throw Exception::interrupted("Target received a shutdown event.");
                    }

                    if (completion)
                    {
                        if (auto dataEntry = completion.value().tryData(); dataEntry)
                        {
                            // The written grain index is sent as immediate data, and was returned
                            // from the completion queue.
                            result.grainAvailable = dataEntry->data();

                            // Need to post receive buffers for immediate data
                            if (_domain->usingRecvBufForCqData())
                            {
                                // Post another receive for the next incoming grain. When another transfer arrives,
                                // the immmediate data (in our case the grain index), will be returned in the registered region.
                                state.ep.recv(state.immData->toLocalRegion());
                            }
                        }
                        else
                        {
                            MXL_ERROR("CQ Error={}", completion->err().toString());
                        }
                    }

                    return state;
                }},
            std::move(_state));

        return result;
    }
}
