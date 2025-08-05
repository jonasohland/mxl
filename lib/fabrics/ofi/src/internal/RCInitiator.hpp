#pragma once

#include <memory>
#include <optional>
#include <vector>
#include "Domain.hpp"
#include "Initiator.hpp"
#include "RegisteredRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    class RCInitiator : public Initiator
    {
    public:
        ~RCInitiator() override = default;

        static std::unique_ptr<RCInitiator> setup(mxlInitiatorConfig const& config);

        void addTarget(TargetInfo const& targetInfo) final;
        void removeTarget(TargetInfo const& targetInfo) final;
        void transferGrain(uint64_t grainIndex) final;

    private:
        std::optional<std::shared_ptr<Domain>> _domain = std::nullopt;
        std::vector<RegisteredRegionGroup> _registeredRegions;
        std::vector<LocalRegionGroup> _localRegions;
        std::map<uuids::uuid, InitiatorTargetEntry> _targets;
    };
}
