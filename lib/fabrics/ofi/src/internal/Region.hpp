#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <bits/types/struct_iovec.h>
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{

    struct Region
    {
        friend std::ostream& operator<<(std::ostream& os, Region const& region);
        friend std::istream& operator>>(std::istream& is, Region& region);

        [[nodiscard]]
        ::iovec to_iovec() const noexcept;

        void* base;
        size_t len;
    };

    class Regions
    {
    public:
        explicit Regions() = default;

        explicit Regions(std::vector<Region> regions)
            : _inner(std::move(regions))
        {}

        friend std::ostream& operator<<(std::ostream& os, Regions const& regions);
        friend std::istream& operator>>(std::istream& is, Regions& regions);

        [[nodiscard]]
        bool empty() const noexcept
        {
            return _inner.empty();
        }

        static Regions* fromAPI(mxlRegions) noexcept;
        ::mxlRegions toAPI() noexcept;

        [[nodiscard]]
        std::vector<::iovec> to_iovec() const noexcept;

    private:
        std::vector<Region> _inner;
    };

}