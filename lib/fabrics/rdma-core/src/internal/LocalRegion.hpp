// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <bits/types/struct_iovec.h>
#include <infiniband/verbs.h>

namespace mxl::lib::fabrics::rdma_core
{
    struct LocalRegion
    {
        std::uint64_t addr;
        std::size_t len;
        std::uint32_t lkey;

        [[nodiscard]]
        ::ibv_sge toSge() const noexcept;
    };

    class LocalRegionGroup
    {
    public:
        LocalRegionGroup(std::vector<LocalRegion> inner)
            : _inner(std::move(inner))
            , _sgl(sglFromGroup(_inner))
        {}

        [[nodiscard]]
        ::ibv_sge const* sgl() const noexcept;

        ::ibv_sge* sgl() noexcept;

        LocalRegion& front() noexcept;

        [[nodiscard]]
        size_t count() const noexcept;

    private:
        static std::vector<::ibv_sge> sglFromGroup(std::vector<LocalRegion> group) noexcept;

    private:
        std::vector<LocalRegion> _inner;
        std::vector<::ibv_sge> _sgl;
    };

}
