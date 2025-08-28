#include "LocalRegion.hpp"
#include <algorithm>
#include <iterator>
#include <vector>
#include <infiniband/verbs.h>

namespace mxl::lib::fabrics::rdma_core
{

    ::ibv_sge LocalRegion::toSge() const noexcept
    {
        return {.addr = addr, .length = static_cast<std::uint32_t>(len), .lkey = lkey};
    }

    ::ibv_sge const* LocalRegionGroup::sgl() const noexcept
    {
        return _sgl.data();
    }

    ::ibv_sge* LocalRegionGroup::sgl() noexcept
    {
        return _sgl.data();
    }

    LocalRegion& LocalRegionGroup::front() noexcept
    {
        return _inner.front();
    }

    std::size_t LocalRegionGroup::count() const noexcept
    {
        return _sgl.size();
    }

    std::vector<::ibv_sge> LocalRegionGroup::sglFromGroup(std::vector<LocalRegion> group) noexcept
    {
        std::vector<::ibv_sge> sgl;
        std::ranges::transform(group, std::back_inserter(sgl), [](LocalRegion& reg) { return reg.toSge(); });
        return sgl;
    }

}
