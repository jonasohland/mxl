#pragma once

#include <memory>
#include <vector>
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "Endpoint.hpp"

namespace mxl::lib::fabrics::ofi
{

    class Initiator
    {
    public:
        // I think we should define our internal objects so that we are decoupled from the public API
        static std::pair<mxlStatus, std::unique_ptr<Initiator>> setup(mxlInitiatorConfig const& config);
        mxlStatus addTarget(mxlTargetInfo const& targetInfo);
        mxlStatus removeTarget(mxlTargetInfo const& targetInfo);

    private:
        Initiator(std::shared_ptr<Domain> domain, std::vector<std::shared_ptr<Endpoint>> endpoints)
            : _domain(std::move(domain))
            , _endpoints(std::move(endpoints))
        {}

        std::shared_ptr<Domain> _domain;
        std::vector<std::shared_ptr<Endpoint>> _endpoints; // Should probably be changed to a map
    };
}