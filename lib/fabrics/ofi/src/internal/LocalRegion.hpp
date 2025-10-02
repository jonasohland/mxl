// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <bits/types/struct_iovec.h>

namespace mxl::lib::fabrics::ofi
{
    class RemoteRegionGroup;

    struct LocalRegion
    {
    public:
        [[nodiscard]]
        ::iovec toIov() const noexcept;

    public:
        std::uint64_t addr;
        std::size_t len;
        void* desc;
    };

    class LocalRegionGroup
    {
    public:
        LocalRegionGroup(std::vector<LocalRegion> inner)
            : _inner(std::move(inner))
            , _iovs(iovFromGroup(_inner))
            , _descs(descFromGroup(_inner))
        {}

        [[nodiscard]]
        std::vector<LocalRegion> const& view() const noexcept;

        [[nodiscard]]
        ::iovec const* iovec() const noexcept;
        [[nodiscard]]
        std::size_t count() const noexcept;

        [[nodiscard]]
        void* const* desc() const noexcept;

    private:
        static std::vector<::iovec> iovFromGroup(std::vector<LocalRegion> group) noexcept;
        static std::vector<void*> descFromGroup(std::vector<LocalRegion> group) noexcept;

    private:
        std::vector<LocalRegion> _inner;

        std::vector<::iovec> _iovs;
        std::vector<void*> _descs;
    };

}
