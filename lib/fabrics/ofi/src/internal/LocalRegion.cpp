// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "LocalRegion.hpp"
#include <algorithm>
#include <sys/uio.h>
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{
    ::iovec LocalRegion::toIovec() const noexcept
    {
        return ::iovec{.iov_base = reinterpret_cast<void*>(addr), .iov_len = len};
    }

    ::iovec const* LocalRegionGroup::asIovec() const noexcept
    {
        return _iovs.data();
    }

    void* const* LocalRegionGroup::desc() const noexcept
    {
        return _descs.data();
    }

    std::vector<::iovec> LocalRegionGroup::iovFromGroup(std::vector<LocalRegion> group) noexcept
    {
        std::vector<::iovec> iovs;
        std::ranges::transform(group, std::back_inserter(iovs), [](LocalRegion const& reg) { return reg.toIovec(); });
        return iovs;
    }

    std::vector<void*> LocalRegionGroup::descFromGroup(std::vector<LocalRegion> group) noexcept
    {
        std::vector<void*> descs;
        std::ranges::transform(group, std::back_inserter(descs), [](LocalRegion& reg) { return reg.desc; });
        return descs;
    }

    LocalRegionGroupSpan LocalRegionGroup::span(std::size_t beginIndex, std::size_t endIndex)
    {
        if (endIndex < beginIndex)
        {
            throw Exception::internal("endIndex {} is smaller than beginIndex {}", endIndex, beginIndex);
        }

        auto spanLength = endIndex - beginIndex;
        if (spanLength > _inner.size())
        {
            throw Exception::internal("requested span size {} will be bigger than the actual size of the full vector {}", spanLength, _inner.size());
        }

        auto begin = &_iovs[beginIndex];
        auto desc = &_descs[beginIndex];

        return LocalRegionGroupSpan{begin, spanLength, desc};
    }

} // namespace mxl::lib::fabrics::ofi
