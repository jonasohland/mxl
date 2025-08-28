#include "RCTarget.hpp"
#include <chrono>
#include <memory>
#include <utility>
#include <infiniband/verbs.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "Endpoint.hpp"
#include "QueueHelpers.hpp"
#include "Region.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    std::pair<std::unique_ptr<RCTarget>, std::unique_ptr<TargetInfo>> RCTarget::setup(mxlTargetConfig const& config)
    {
        MXL_INFO("setting up target [endpoint = {}:{}]", config.endpointAddress.node, config.endpointAddress.service);
        auto bindAddr = Address{config.endpointAddress.node, config.endpointAddress.service};

        MXL_INFO("created bindAddr: {}", bindAddr.toString());

        auto pep = PassiveEndpoint::create(bindAddr);

        pep.connectionManagement().protectionDomain().registerRegionGroups(*RegionGroups::fromAPI(config.regions),
            IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);

        auto remoteRegions = pep.connectionManagement().protectionDomain().remoteRegions();
        for (auto const& region : remoteRegions)
        {
            MXL_INFO("RemoteRegion -> addr=0x{:x} rkey={:x}", region.addr, region.rkey);
        }

        // Helper struct to enable the std::make_unique function to access the private constructor of this class
        struct MakeUniqueEnabler : RCTarget
        {
            MakeUniqueEnabler(State state)
                : RCTarget(std::move(state))
            {}
        };

        auto targetInfo = TargetInfo{bindAddr, pep.connectionManagement().protectionDomain().remoteRegions()};

        return {std::make_unique<MakeUniqueEnabler>(WaitForConnectionRequest{std::move(pep)}), std::make_unique<TargetInfo>(std::move(targetInfo))};
    }

    RCTarget::ReadResult RCTarget::read()
    {
        return makeProgress<QueueReadMode::NonBlocking>({});
    }

    RCTarget::ReadResult RCTarget::readBlocking(std::chrono::steady_clock::duration timeout)
    {
        return makeProgress<QueueReadMode::Blocking>(timeout);
    }

    RCTarget::RCTarget(State state)
        : _state(std::move(state))
    {}

    template<QueueReadMode qrm>
    RCTarget::ReadResult RCTarget::makeProgress(std::chrono::steady_clock::duration timeout)
    {
        Target::ReadResult result = {};

        _state = std::visit(
            overloaded{
                [](std::monostate) -> State { throw std::runtime_error("Target is in an invalid state and can no longer make progress"); },
                [](WaitForConnectionRequest state) -> State
                {
                    state.pep.listen();
                    auto aep = state.pep.waitConnectionRequest();
                    MXL_INFO("Connected!");

                    auto immData = ImmediateDataLocation(aep.connectionManagement().protectionDomain());
                    MXL_INFO("Created immData");
                    auto immRegion = immData.toLocalRegion();
                    aep.recv(immRegion);
                    MXL_INFO("Posted imm data recv");

                    return Connected{.ep = std::move(aep), ._immData = std::move(immData)};
                },
                [&](Connected state) -> State
                {
                    MXL_INFO("Polling the completion queue");
                    auto completion = readCompletionQueue<qrm>(state.ep, timeout);
                    if (completion)
                    {
                        if (completion.value().isErr())
                        {
                            MXL_ERROR("CQ Error: {}", completion.value().errToString());
                        }
                        else
                        {
                            result.grainAvailable = completion.value().wrId();
                            auto immRegion = state._immData.toLocalRegion();
                            state.ep.recv(immRegion);
                            MXL_INFO("Posted imm data recv");
                        }
                    }

                    MXL_INFO("Done polling");

                    return std::move(state);
                },
            },
            std::move(_state));

        return result;
    }

}
