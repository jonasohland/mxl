#pragma once

#include <memory>
#include <utility>
#include <mxl/fabrics.h>
#include <mxl/mxl.h>
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Target
    {
    public:
        virtual ~Target()
        {}

        [[nodiscard]]
        virtual TargetInfo getInfo() const = 0;
    };

    class TargetWrapper
    {
    public:
        TargetWrapper();
        ~TargetWrapper();

        std::pair<mxlStatus, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const& config);

    private:
        std::unique_ptr<Target> _inner;
    };
}
