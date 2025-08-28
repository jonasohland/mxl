// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "Target.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
#include <fmt/format.h>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "mxl/fabrics.h"
#include "RCTarget.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    TargetWrapper* TargetWrapper::fromAPI(mxlFabricsTarget api) noexcept
    {
        return reinterpret_cast<TargetWrapper*>(api);
    }

    mxlFabricsTarget TargetWrapper::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsTarget>(this);
    }

    Target::ReadResult TargetWrapper::read()
    {
        if (!_inner)
        {
            throw std::runtime_error("Target is not set up");
        }

        return _inner->read();
    }

    Target::ReadResult TargetWrapper::readBlocking(std::chrono::steady_clock::duration timeout)
    {
        if (!_inner)
        {
            throw std::runtime_error("Target is not set up");
        }

        return _inner->readBlocking(timeout);
    }

    std::unique_ptr<TargetInfo> TargetWrapper::setup(mxlTargetConfig const& config)
    {
        if (_inner)
        {
            _inner.release();
        }

        auto [target, info] = RCTarget::setup(config);
        _inner = std::move(target);
        return std::move(info);
    }

}
