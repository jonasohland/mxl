#include "RCInitiator.hpp"
#include <chrono>
#include <stdexcept>
#include <variant>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "Address.hpp"
#include "ConnectionManagement.hpp"
#include "Endpoint.hpp"
#include "LocalRegion.hpp"
#include "TargetInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    std::unique_ptr<RCInitiator> RCInitiator::setup(mxlInitiatorConfig const& config)
    {
        auto bindAddr = Address{config.endpointAddress.node, config.endpointAddress.service};

        auto pep = PassiveEndpoint::create(bindAddr);
        MXL_INFO("Created PassiveEndpoint");

        pep.connectionManagement().protectionDomain().registerRegionGroups(*RegionGroups::fromAPI(config.regions),
            0); // Local read access is always enabled for the MR.

        // Helper struct to enable the std::make_unique function to access the private constructor of this class
        struct MakeUniqueEnabler : RCInitiator
        {
            MakeUniqueEnabler(State state, std::vector<LocalRegionGroup> localRegions)
                : RCInitiator(std::move(state), std::move(localRegions))
            {}
        };

        return std::make_unique<MakeUniqueEnabler>(Uninitialized{}, std::vector<LocalRegionGroup>{});
    }

    void RCInitiator::addTarget(TargetInfo const& targetInfo)
    {
        MXL_INFO("Add Target");
        _state = std::visit(
            overloaded{
                [](Uninitialized) -> State
                { throw std::runtime_error("Initiator needs to be initialized by calling the setup function before trying to add targets."); },
                [&](Idle state) -> State
                {
                    auto dstAddr = targetInfo.addr;
                    return Connected{.ep = state.pep.connect(dstAddr), .addr = dstAddr, .regions = targetInfo.remoteRegions};
                },
                [](Connected) -> State { throw std::runtime_error("Currently the implementation does not support more than 1 target at a time."); },
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

            state->ep.write(grainIndex, local, remote);
        }
        else
        {
            throw std::runtime_error("A target needs to be added before you attempt to transfer a grain.");
        }
    }

    bool RCInitiator::makeProgress()
    {
        if (auto state = std::get_if<Connected>(&_state); state)
        {
            if (auto completion = state->ep.readCq(); completion)
            {
                return true;
            }
        }
        return false;
    }

    bool RCInitiator::makeProgressBlocking(std::chrono::steady_clock::duration)
    {
        if (auto state = std::get_if<Connected>(&_state); state)
        {
            if (auto completion = state->ep.readCqBlocking(); completion)
            {
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
