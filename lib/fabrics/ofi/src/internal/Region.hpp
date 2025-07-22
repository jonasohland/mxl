#pragma once

#include <array>
#include <istream>
#include <ostream>
#include <variant>
#include <vector>
#include <uuid.h>
#include <bits/types/struct_iovec.h>
#include "internal/FlowData.hpp"
#include "internal/Instance.hpp"
#include "internal/SharedMemory.hpp"
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    using Buffers = std::array<std::pair<void*, std::size_t>, 2>;

    struct Region
    {
        Region(Buffers buffers)
            : buffers(std::move(buffers))
        {}

        friend std::ostream& operator<<(std::ostream& os, Region const& region);
        friend std::istream& operator>>(std::istream& is, Region& region);

        [[nodiscard]]
        std::array<::iovec, 2> to_iovec() const noexcept;

        Buffers buffers;
    };

    class Regions
    {
    public:
        explicit Regions() = default;

        explicit Regions(std::vector<Region> regions)
            : _inner(std::move(regions))
        {}

        static Regions fromFlow(FlowData* flow);

        friend std::ostream& operator<<(std::ostream& os, Regions const& regions);
        friend std::istream& operator>>(std::istream& is, Regions& regions);

        [[nodiscard]]
        bool empty() const noexcept;

        static Regions* fromAPI(mxlRegions) noexcept;
        ::mxlRegions toAPI() noexcept;

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

    std::vector<Region> buildRegions(void** buffers, std::size_t* sizes, std::size_t count);
}
