// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "FabricsInstance.hpp"
#include <stdexcept>
#include <internal/Logging.hpp>
#include "mxl/fabrics.h"
#include "Initiator.hpp"
#include "Target.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    FabricsInstance::FabricsInstance(mxl::lib::Instance* instance)
        : _mxlInstance(instance)
    {
        MXL_FABRICS_UNUSED(_mxlInstance);
    }

    mxlFabricsInstance FabricsInstance::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsInstance>(this);
    }

    FabricsInstance* FabricsInstance::fromAPI(mxlFabricsInstance instance) noexcept
    {
        return reinterpret_cast<FabricsInstance*>(instance);
    }

    TargetWrapper* FabricsInstance::createTarget()
    {
        return &_targets.emplace_back();
    }

    InitiatorWrapper* FabricsInstance::createInitiator()
    {
        return &_initiators.emplace_back();
    }

    void FabricsInstance::destroyTarget(TargetWrapper* wrapper)
    {
        if (!_targets.remove_if([&](TargetWrapper const& lhs) { return &lhs == wrapper; }))
        {
            throw std::runtime_error("Target to remove is not known to instance");
        }
    }

    void FabricsInstance::destroyInitiator(InitiatorWrapper* initiator)
    {
        if (!_initiators.remove_if([&](InitiatorWrapper const& lhs) { return &lhs == initiator; }))
        {
            throw std::runtime_error("Initiator to remove is not known to instance");
        }
    }

}
