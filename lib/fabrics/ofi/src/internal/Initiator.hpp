#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <uuid.h>
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "Endpoint.hpp"
#include "RegisteredRegion.hpp"
#include "RemoteRegion.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct InitiatorTargetEntry
    {
        std::shared_ptr<Endpoint> endpoint;
        std::vector<RemoteRegionGroup> remoteGroups;
    };

    class Initiator final
    {
    public:
        Initiator() = default;
        ~Initiator();

        static Initiator* fromAPI(mxlFabricsInitiator api);

        mxlFabricsInitiator toAPI() noexcept;

        mxlStatus setup(mxlInitiatorConfig const& config);
        mxlStatus addTarget(TargetInfo const& targetInfo);
        mxlStatus removeTarget(TargetInfo const& targetInfo);

        mxlStatus transferGrain(uint64_t grainIndex);

    private:
        std::optional<std::shared_ptr<Domain>> _domain = std::nullopt;
        std::vector<RegisteredRegionGroup> _registeredRegions;
        std::vector<LocalRegionGroup> _localRegions;
        std::map<uuids::uuid, InitiatorTargetEntry> _targets;
    };
}
