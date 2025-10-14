// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "RegisteredRegion.hpp"
#include "LocalRegion.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    RemoteRegion RegisteredRegion::toRemote(bool useVirtualAddress) const noexcept
    {
        auto addr = useVirtualAddress ? _region.base : 0;

        return RemoteRegion{.addr = addr, .len = _region.size, .rkey = _mr.rkey()};
    }

    LocalRegion RegisteredRegion::toLocal() const noexcept
    {
        return LocalRegion{.addr = _region.base, .len = _region.size, .desc = _mr.desc()};
    }

    std::vector<RemoteRegion> toRemote(std::vector<RegisteredRegion> const& regions, bool useVirtualAddress) noexcept
    {
        std::vector<RemoteRegion> remoteRegions;
        std::ranges::transform(
            regions, std::back_inserter(remoteRegions), [&](RegisteredRegion const& reg) { return reg.toRemote(useVirtualAddress); });
        return remoteRegions;
    }

    std::vector<LocalRegion> toLocal(std::vector<RegisteredRegion> const& regions) noexcept
    {
        std::vector<LocalRegion> localRegions;
        std::ranges::transform(regions, std::back_inserter(localRegions), [](RegisteredRegion const& reg) { return reg.toLocal(); });
        return localRegions;
    }

} // namespace mxl::lib::fabrics::ofi
