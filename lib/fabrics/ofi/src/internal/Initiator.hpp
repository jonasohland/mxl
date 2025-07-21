#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include "mxl/fabrics.h"
#include "mxl/flow.h"
#include "mxl/mxl.h"
#include "Endpoint.hpp"
#include "MemoryRegion.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct InitiatorTargetEntry
    {
        std::shared_ptr<Endpoint> _endpoint;
        Regions _regions;
        uint64_t _rkey;
    };

    class Initiator
    {
    public:
        Initiator() = default;

        // TODO:: we should define our internal objects so that we are decoupled from the public API
        mxlStatus setup(mxlInitiatorConfig const& config);
        mxlStatus addTarget(std::string identifier, TargetInfo const& targetInfo);
        mxlStatus removeTarget(std::string identifier, TargetInfo const& targetInfo);

        mxlStatus transferGrain(uint64_t grainIndex, GrainInfo const* grainInfo, uint8_t const* payload);
        mxlStatus transferGrainToTarget(std::string identifer, uint64_t grainIndex, GrainInfo const* grainInfo, uint8_t const* payload);

    private:
        std::optional<std::shared_ptr<Domain>> _domain = std::nullopt;
        std::optional<std::shared_ptr<MemoryRegion>> _mr = std::nullopt;
        std::map<std::string, InitiatorTargetEntry> _targets;
    };
}
