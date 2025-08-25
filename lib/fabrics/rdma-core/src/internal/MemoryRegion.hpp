#pragma once

#include <rdma/rdma_verbs.h>
#include "Endpoint.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class MemoryRegion
    {
    public:
        static MemoryRegion reg(Endpoint& ep, Region const& region);

        ~MemoryRegion();

        MemoryRegion(MemoryRegion const&) = delete;
        void operator=(MemoryRegion const&) = delete;

        MemoryRegion(MemoryRegion&&) noexcept;
        MemoryRegion& operator=(MemoryRegion&&) noexcept;

        ::ibv_mr* raw() noexcept;
        [[nodiscard]]
        ::ibv_mr const* raw() const noexcept;

        [[nodiscard]]
        std::uint32_t lkey() const noexcept;

        [[nodiscard]]
        std::uint32_t rkey() const noexcept;

    private:
        friend class RegisteredRegion;

        void close();

        MemoryRegion(::ibv_mr* raw);

        ::ibv_mr* _raw;
    };

}
