// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <bits/types/struct_iovec.h>

namespace mxl::lib::fabrics::ofi
{
    class RemoteRegionGroup;
    class LocalRegionGroup;
    class LocalRegionGroupSpan;

    struct LocalRegion
    {
    public:
        [[nodiscard]]
        ::iovec toIovec() const noexcept;

        /// The returned LocalRegionGroup will contain this single LocalRegion.
        [[nodiscard]]
        LocalRegionGroup asGroup() const noexcept;

    public:
        std::uint64_t addr;
        std::size_t len;
        void* desc;
    };

    /// Used when the user wants to transfer many non-contiguous regions in a single transfer.
    class LocalRegionGroup
    {
    public:
        using iterator = std::vector<LocalRegion>::iterator;
        using const_iterator = std::vector<LocalRegion>::const_iterator;

    public:
        LocalRegionGroup(std::vector<LocalRegion> inner)
            : _inner(std::move(inner))
            , _iovs(iovFromGroup(_inner))
            , _descs(descFromGroup(_inner))
        {}

        [[nodiscard]]
        ::iovec const* asIovec() const noexcept;
        [[nodiscard]]
        void* const* desc() const noexcept;

        iterator begin()
        {
            return _inner.begin();
        }

        iterator end()
        {
            return _inner.end();
        }

        [[nodiscard]]
        const_iterator begin() const
        {
            return _inner.cbegin();
        }

        [[nodiscard]]
        const_iterator end() const
        {
            return _inner.cend();
        }

        LocalRegion& operator[](std::size_t index)
        {
            return _inner[index];
        }

        LocalRegion const& operator[](std::size_t index) const
        {
            return _inner[index];
        }

        [[nodiscard]]
        size_t size() const noexcept
        {
            return _inner.size();
        }

        [[nodiscard]]
        LocalRegionGroupSpan span(std::size_t beginIndex, std::size_t endIndex) const;

    private:
        static std::vector<::iovec> iovFromGroup(std::vector<LocalRegion> group) noexcept;
        static std::vector<void*> descFromGroup(std::vector<LocalRegion> group) noexcept;

    private:
        std::vector<LocalRegion> _inner;

        std::vector<::iovec> _iovs;
        std::vector<void*> _descs;
    };

    class LocalRegionGroupSpan
    {
    public:
        [[nodiscard]]
        ::iovec const* asIovec() const noexcept
        {
            return _iovec.data();
        }

        [[nodiscard]]
        void* const* desc() const noexcept
        {
            return _descs.data();
        }

        [[nodiscard]]
        std::size_t size() const noexcept
        {
            return _inner.size();
        }

        /// Returns the sum of all region "len" in the span which corresponds to the total amount of bytes all non-contiguous buffers uses.
        [[nodiscard]]
        std::size_t byteSize() const noexcept;

    private:
        friend class LocalRegionGroup;

    private:
        LocalRegionGroupSpan(std::span<LocalRegion const>, std::span<::iovec const>, std::span<void* const>);

    private:
        std::span<LocalRegion const> _inner;
        std::span<::iovec const> _iovec;
        std::span<void* const> _descs;
    };
}
