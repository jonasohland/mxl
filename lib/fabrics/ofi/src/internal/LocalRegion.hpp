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
    class LocalRegionGroupSpan;

    struct LocalRegion
    {
    public:
        [[nodiscard]]
        ::iovec toIovec() const noexcept;

    public:
        std::uint64_t addr;
        std::size_t len;
        void* desc;
    };

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

        LocalRegionGroupSpan span(size_t beginIndex, size_t endIndex);

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
            return _inner;
        }

        [[nodiscard]]
        void* const* desc() const noexcept
        {
            return _desc;
        }

        [[nodiscard]]
        std::size_t size() const noexcept
        {
            return _size;
        }

    private:
        friend class LocalRegionGroup;

    private:
        LocalRegionGroupSpan(::iovec const* begin, std::size_t size, void* const* desc)
            : _inner(begin)
            , _size(size)
            , _desc(desc)
        {}

    private:
        ::iovec const* _inner;
        std::size_t _size;
        void* const* _desc;
    };

}
