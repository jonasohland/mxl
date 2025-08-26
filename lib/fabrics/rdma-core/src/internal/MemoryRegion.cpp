// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "MemoryRegion.hpp"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>
#include "internal/Logging.hpp"
// #include "Exception.hpp"
#include "Endpoint.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    MemoryRegion MemoryRegion::reg(Endpoint& ep, Region const& region, uint64_t access)
    {
        MXL_DEBUG("Registering memory region with address 0x{}, size {} and location {}",
            reinterpret_cast<void*>(region.base),
            region.size,
            region.loc.toString());

        auto mr = ibv_reg_mr(ep.raw()->pd, reinterpret_cast<void*>(region.base), region.size, access);
        if (mr == nullptr)
        {
            throw std::runtime_error(
                fmt::format("failed to register buffer for remote write operation: {}", strerror(errno))); // TODO: replace with internal exception
        }

        struct MakeSharedEnabler : public MemoryRegion
        {
            MakeSharedEnabler(::ibv_mr* raw)
                : MemoryRegion(raw)
            {}
        };

        return {mr};
    }

    MemoryRegion::~MemoryRegion()
    {
        close();
    }

    MemoryRegion::MemoryRegion(MemoryRegion&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    MemoryRegion& MemoryRegion::operator=(MemoryRegion&& other) noexcept
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    std::uint32_t MemoryRegion::lkey() const noexcept
    {
        return _raw->lkey;
    }

    std::uint32_t MemoryRegion::rkey() const noexcept
    {
        return _raw->rkey;
    }

    MemoryRegion::MemoryRegion(::ibv_mr* raw)
        : _raw(raw)
    {}

    void MemoryRegion::close()
    {
        if (_raw)
        {
            MXL_DEBUG("Closing memory region with rkey={:x} lkey:{:x}", rkey(), lkey());

            if (ibv_dereg_mr(_raw) != 0)
            {
                throw std::runtime_error(fmt::format("failed to de-register buffer: {}", strerror(errno))); // TODO: replace internal exception
            }
            _raw = nullptr;
        }
    }

}
