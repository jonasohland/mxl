#include "FabricsInstance.hpp"
#include <internal/Logging.hpp>
#include "Exception.hpp"
#include "FILogging.hpp"

namespace mxl::lib::fabrics::ofi
{
    FabricsInstance::FabricsInstance(mxl::lib::Instance* instance)
        : _mxlInstance(instance)
    {
        fiInitLogging();
    }

    FabricsInstance::~FabricsInstance()
    {}

    TargetWrapper* FabricsInstance::createTarget()
    {
        return &_targets.emplace_back();
    }

    void FabricsInstance::destroyTarget(TargetWrapper* wrapper)
    {
        if (!_targets.remove_if([&](TargetWrapper const& lhs) { return &lhs == wrapper; }))
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "Target to remove is not known to instance");
        }
    }

}
