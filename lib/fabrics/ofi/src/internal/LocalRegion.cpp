// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "LocalRegion.hpp"
#include <algorithm>
#include <numeric>
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{
    ::iovec LocalRegion::toIovec() const noexcept
    {
        return ::iovec{.iov_base = reinterpret_cast<void*>(addr), .iov_len = len};
    }

    LocalRegionGroup LocalRegion::asGroup() const noexcept
    {
        return {std::vector<LocalRegion>{*this}};
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

    LocalRegionGroupSpan LocalRegionGroup::span(std::size_t begin, std::size_t end) const
    {
        if (end < begin)
        {
            throw Exception::internal("end {} is smaller than begin {}", end, begin);
        }

        auto spanLength = end - begin;
        if (spanLength > _inner.size())
        {
            throw Exception::internal("requested span size {} will be bigger than the actual size of the full vector {}", spanLength, _inner.size());
        }

        return LocalRegionGroupSpan{
            std::span(_inner.begin() + begin, spanLength),
            std::span(_iovs.begin() + begin, spanLength),
            std::span(_descs.begin() + begin, spanLength),
        };
    }

    LocalRegionGroupSpan::LocalRegionGroupSpan(std::span<LocalRegion const> region, std::span<::iovec const> iovec, std::span<void* const> desc)
        : _inner(region)
        , _iovec(iovec)
        , _descs(desc)
    {}

    std::size_t LocalRegionGroupSpan::byteSize() const noexcept
    {
        return std::accumulate(_inner.begin(), _inner.end(), 0, [](size_t sum, auto const region) { return sum + region.len; });
    }
} // namespace mxl::lib::fabrics::ofi
