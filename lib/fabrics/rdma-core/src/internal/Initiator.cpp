// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "Initiator.hpp"
#include <stdexcept>
#include "mxl/fabrics.h"
#include "Exception.hpp"
#include "RCInitiator.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    InitiatorWrapper* InitiatorWrapper::fromAPI(mxlFabricsInitiator api) noexcept
    {
        return reinterpret_cast<InitiatorWrapper*>(api);
    }

    mxlFabricsInitiator InitiatorWrapper::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsInitiator>(this);
    }

    void InitiatorWrapper::setup(mxlInitiatorConfig const& config)
    {
        if (_inner)
        {
            _inner.release();
        }

        _inner = RCInitiator::setup(config);
    }

    void InitiatorWrapper::addTarget(TargetInfo const& targetInfo)
    {
        if (!_inner)
        {
            throw std::runtime_error("Initiator is not set up");
        }

        _inner->addTarget(targetInfo);
    }

    void InitiatorWrapper::removeTarget(TargetInfo const& targetInfo)
    {
        if (!_inner)
        {
            throw std::runtime_error("Initiator is not set up");
        }

        _inner->removeTarget(targetInfo);
    }

    void InitiatorWrapper::transferGrain(uint64_t grainIndex)
    {
        if (!_inner)
        {
            throw Exception::internal("Initiator is not set up");
        }

        _inner->transferGrain(grainIndex);
    }

    bool InitiatorWrapper::makeProgress()
    {
        if (!_inner)
        {
            throw std::runtime_error("Initiator is not set up");
        }

        return _inner->makeProgress();
    }

    bool InitiatorWrapper::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        if (!_inner)
        {
            throw std::runtime_error("Initiator is not set up");
        }

        return _inner->makeProgressBlocking(timeout);
    }

}
