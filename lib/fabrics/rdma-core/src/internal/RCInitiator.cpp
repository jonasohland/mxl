#include "RCInitiator.hpp"
#include <chrono>
#include <stdexcept>
#include <variant>
#include <infiniband/verbs.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "ConnectionManagement.hpp"
#include "Exception.hpp"
#include "LocalRegion.hpp"
#include "QueuePair.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto bindAddr = Address{config.endpointAddress.node, config.endpointAddress.service};

        auto cm = ConnectionManagement::create();
        cm.bind(bindAddr);

        cm.createProtectionDomain();
        cm.pd().registerRegionGroups(*RegionGroups::fromAPI(config.regions),
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ); // Local read access is always enabled for the MR.

        MXL_INFO("Success to register memory");

        auto localRegions = cm.pd().localRegionGroups();
        for (auto const& region : localRegions)
        {
            MXL_INFO("LocalRegion -> addr=0x{:x} len={} lkey=0x{:x} ", region.sgl()->addr, region.sgl()->length, region.sgl()->lkey);
        }

        // Helper struct to enable the std::make_unique function to access the private constructor of this class
        struct MakeUniqueEnabler : RCInitiator
        {
            MakeUniqueEnabler(State state, std::vector<LocalRegionGroup> localRegions)
                : RCInitiator(std::move(state), std::move(localRegions))
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(Idle{std::move(cm)}, std::move(localRegions));
    }

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        MXL_INFO("Add Target {}", targetInfo.addr.toString());
        _state = std::visit(
            overloaded{
                [](Uninitialized) -> State
                { throw std::runtime_error("Initiator needs to be initialized by calling the setup function before trying to add targets."); },
                [&](Idle state) -> State
                {
                    auto dstAddr = targetInfo.addr;

                    // Prepare connection
                    state.cm.resolveAddr(dstAddr, std::chrono::seconds(2));
                    state.cm.resolveRoute(std::chrono::seconds(2));
                    state.cm.createCompletionQueue();
                    state.cm.createQueuePair(QueuePairAttr::defaults());

                    state.cm.connect();

                    return Connected{.cm = std::move(state.cm), .regions = targetInfo.remoteRegions};
                },
                [](Connected) -> State { throw Exception::internal("Currently the implementation does not support more than 1 target at a time."); },
            },
            std::move(_state));
    }

    void RCInitiator::removeTarget(TargetInfo const&)
    {
        // NOP
        MXL_WARN("Currently the implementation does not support removing a target, create a new initiator instead.");
    }

    void RCInitiator::transferGrain(std::uint64_t grainIndex)
    {
        if (auto state = std::get_if<Connected>(&_state); state)
        {
            auto& remote = state->regions[grainIndex % state->regions.size()];
            auto& local = _localRegions[grainIndex % _localRegions.size()].front();

            state->cm.write(grainIndex, local, remote);
            pendingTransfer++;
        }
        else
        {
            throw std::runtime_error("A target needs to be added before you attempt to transfer a grain.");
        }
    }

    bool RCInitiator::makeProgress()
    {
        if (auto state = std::get_if<Connected>(&_state); state && pendingTransfer > 0)
        {
            if (auto completion = state->cm.readCq(); completion)
            {
                pendingTransfer--;
                return true;
            }
        }
        return false;
    }

    bool RCInitiator::makeProgressBlocking(std::chrono::steady_clock::duration)
    {
        if (auto state = std::get_if<Connected>(&_state); state && pendingTransfer > 0)
        {
            if (auto completion = state->cm.readCqBlocking(); completion)
            {
                pendingTransfer--;
                return true;
            }
        }
        return false;
    }

    RCInitiator::RCInitiator(State state, std::vector<LocalRegionGroup> localRegions)
        : _localRegions(std::move(localRegions))
        , _state(std::move(state))
    {}
}
