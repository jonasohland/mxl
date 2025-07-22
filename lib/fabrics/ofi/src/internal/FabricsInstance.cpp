#include "FabricsInstance.hpp"
#include <internal/Logging.hpp>
#include "Exception.hpp"
#include "FILogging.hpp"
#include "Initiator.hpp"
#include "Target.hpp"

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

    Initiator* FabricsInstance::createInitiator()
    {
        return &_initiators.emplace_back();
    }

    void FabricsInstance::destroyTarget(TargetWrapper* wrapper)
    {
        if (!_targets.remove_if([&](TargetWrapper const& lhs) { return &lhs == wrapper; }))
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "Target to remove is not known to instance");
        }
    }

    void FabricsInstance::destroyInitiator(Initiator* initiator)
    {
        if (!_initiators.remove_if([&](Initiator const& lhs) { return &lhs == initiator; }))
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "Initiator to remove is not known to instance");
        }
    }

}
