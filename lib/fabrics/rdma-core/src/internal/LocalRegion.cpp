#include "LocalRegion.hpp"
#include <infiniband/verbs.h>

namespace mxl::lib::fabrics::rdma_core
{

    ::ibv_sge LocalRegion::toSge() const noexcept
    {
        return {.addr = addr, .length = len, .lkey = lkey};
    }

    ::ibv_sge const* LocalRegionGroup::sgl() const noexcept
    {
        return _sgl.data();
    }

    ::ibv_sge* LocalRegionGroup::sgl() noexcept
    {
        return _sgl.data();
    }

    std::size_t LocalRegionGroup::count() const noexcept
    {
        return _sgl.size();
    }

}
