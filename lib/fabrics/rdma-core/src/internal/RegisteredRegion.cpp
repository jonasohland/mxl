// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "RegisteredRegion.hpp"
#include <stdexcept>
#include "LocalRegion.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    RemoteRegion RegisteredRegion::toRemote() const noexcept
    {
        return RemoteRegion{.addr = _region.base, .rkey = _mr.rkey()};
    }

    LocalRegion RegisteredRegion::toLocal() noexcept
    {
        return LocalRegion{
            .addr = _region.base,
            .len = static_cast<std::uint32_t>(_region.size),
            .lkey = _mr.lkey(),
        };
    }

    RemoteRegion RegisteredRegionGroup::toRemote() const noexcept
    {
        std::vector<RemoteRegion> group;

        if (_inner.size() != 1)
        {
            std::runtime_error("only 1 remote region is supported (scatter-gather remote regions are not supported)!");
        }

        return _inner.front().toRemote();
    }

    LocalRegionGroup RegisteredRegionGroup::toLocal() noexcept
    {
        std::vector<LocalRegion> group;

        std::ranges::transform(_inner, std::back_inserter(group), [](RegisteredRegion& reg) { return reg.toLocal(); });

        return LocalRegionGroup{group};
    }

    std::vector<RemoteRegion> toRemote(std::vector<RegisteredRegionGroup> const& groups) noexcept
    {
        std::vector<RemoteRegion> remoteGroups;
        std::ranges::transform(groups, std::back_inserter(remoteGroups), [&](RegisteredRegionGroup const& reg) { return reg.toRemote(); });
        return remoteGroups;
    }

    std::vector<LocalRegionGroup> toLocal(std::vector<RegisteredRegionGroup>& groups) noexcept
    {
        std::vector<LocalRegionGroup> localGroups;
        std::ranges::transform(groups, std::back_inserter(localGroups), [](RegisteredRegionGroup& reg) { return reg.toLocal(); });
        return localGroups;
    }

} // namespace mxl::lib::fabrics::ofi
