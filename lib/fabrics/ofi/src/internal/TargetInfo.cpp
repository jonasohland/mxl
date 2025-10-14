// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "TargetInfo.hpp"
#include <uuid/uuid.h>

namespace mxl::lib::fabrics::ofi
{

    TargetInfo* TargetInfo::fromAPI(mxlTargetInfo api) noexcept
    {
        return reinterpret_cast<TargetInfo*>(api);
    }

    ::mxlTargetInfo TargetInfo::toAPI() noexcept
    {
        return reinterpret_cast<mxlTargetInfo>(this);
    }

    bool TargetInfo::operator==(TargetInfo const& other) const noexcept
    {
        return fabricAddress == other.fabricAddress && remoteRegions == other.remoteRegions && id == other.id;
    }

}
