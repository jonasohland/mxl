#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <variant>
#include <vector>
#include <uuid.h>
#include <bits/types/struct_iovec.h>
#include "internal/Instance.hpp"
#include "internal/SharedMemory.hpp"
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    struct BufferSpace
    {
        std::uintptr_t base;
        size_t size;

        [[nodiscard]]
        ::iovec to_iovec() const;

        friend std::ostream& operator<<(std::ostream& os, BufferSpace const& region);
        friend std::istream& operator>>(std::istream& is, BufferSpace& region);
    };

    class Region
    {
    public:
        friend std::ostream& operator<<(std::ostream& os, Region const& region);
        friend std::istream& operator>>(std::istream& is, Region& region);

        explicit Region() = default;

        explicit Region(std::vector<BufferSpace> inner)
            : _inner(std::move(inner))
        {}

        [[nodiscard]]
        std::uintptr_t firstBaseAddress() const noexcept;

        [[nodiscard]]
        std::vector<::iovec> to_iovec() const noexcept;

    private:
        std::vector<BufferSpace> _inner;
    };

    class Regions
    {
    public:
        explicit Regions() = default;

        explicit Regions(std::vector<Region> regions)
            : _inner(std::move(regions))
        {}

        static Regions fromFlow(FlowData* flow);

        [[nodiscard]]
        Region const& at(size_t index) const noexcept;

        auto begin() noexcept
        {
            return _inner.begin();
        }

        auto end() noexcept
        {
            return _inner.end();
        }

        [[nodiscard]]
        size_t size() const;

        [[nodiscard]]
        bool empty() const noexcept;

        static Regions* fromAPI(mxlRegions) noexcept;
        ::mxlRegions toAPI() noexcept;

        friend std::ostream& operator<<(std::ostream& os, Regions const& regions);
        friend std::istream& operator>>(std::istream& is, Regions& regions);

    private:
        std::vector<Region> _inner;
    };

    class DeferredRegions
    {
    public:
        explicit DeferredRegions(uuids::uuid) noexcept;
        explicit DeferredRegions(Regions) noexcept;

        Regions unwrap(Instance&, AccessMode accessMode);

    private:
        std::variant<uuids::uuid, Regions> _inner;
    };

}
