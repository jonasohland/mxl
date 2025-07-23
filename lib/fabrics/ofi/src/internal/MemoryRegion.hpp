#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <bits/types/struct_iovec.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include "Domain.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    class MemoryRegion
    {
    public:
        static std::shared_ptr<MemoryRegion> reg(std::shared_ptr<Domain> domain, Region const& region, uint64_t access);

        ~MemoryRegion();

        MemoryRegion(MemoryRegion const&) = delete;
        void operator=(MemoryRegion const&) = delete;

        MemoryRegion(MemoryRegion&&) noexcept;
        MemoryRegion& operator=(MemoryRegion&&) noexcept;

        ::fid_mr* raw() noexcept;
        [[nodiscard]]
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

    class RegisteredRegion
    {
    public:
        RegisteredRegion(std::shared_ptr<MemoryRegion> mr, Region region)
            : _mr(std::move(mr))
            , _region(std::move(region))
        {}

        Region& region()
        {
            return _region;
        }

        void* desc()
        {
            return _mr->getLocalMemoryDescriptor();
        }

        uint64_t rkey()
        {
            return _mr->getRemoteKey();
        }

    private:
        std::shared_ptr<MemoryRegion> _mr;

        Region _region;
    };
}
