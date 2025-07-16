#pragma once

#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>
#include <bits/types/struct_iovec.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include "Domain.hpp"

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

        [[nodiscard]]
        std::vector<::iovec> to_iovec() const noexcept;

    private:
        std::vector<Region> _inner;
    };

    class MemoryRegion
    {
    public:
        static std::shared_ptr<MemoryRegion> reg(std::shared_ptr<Domain> domain, Regions const& regions, uint64_t access);

        ~MemoryRegion();

        MemoryRegion(MemoryRegion const&) = delete;
        void operator=(MemoryRegion const&) = delete;

        MemoryRegion(MemoryRegion&&) noexcept;
        MemoryRegion& operator=(MemoryRegion&&) noexcept;

        ::fid_mr* raw() noexcept;
        ::fid_mr const* raw() const noexcept;

        [[nodiscard]]
        void* getLocalMemoryDescriptor() const noexcept;

        [[nodiscard]]
        uint64_t getRemoteKey() const noexcept;

    private:
        void close();

        MemoryRegion(::fid_mr* raw, std::shared_ptr<Domain> domain);

        ::fid_mr* _raw;
        std::shared_ptr<Domain> _domain;
    };
}