#include "ProtectionDomain.hpp"
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include "ConnectionManagement.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    ProtectionDomain::ProtectionDomain(ConnectionManagement& cm)
    {
        _raw = ibv_alloc_pd(cm.raw()->verbs);
        if (_raw == nullptr)
        {
            throw std::runtime_error(fmt::format("Failed to allocate protection domain: {}", strerror(errno)));
        }
    }

    ::ibv_pd* ProtectionDomain::raw() noexcept
    {
        return _raw;
    }

    ProtectionDomain::~ProtectionDomain()
    {
        close();
    }

    ProtectionDomain::ProtectionDomain(ProtectionDomain&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    ProtectionDomain& ProtectionDomain::operator=(ProtectionDomain&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    void ProtectionDomain::close()
    {
        if (_raw)
        {
            if (ibv_dealloc_pd(_raw))
            {
                throw std::runtime_error(fmt::format("Failed to deallocate protection domain: {}", strerror(errno)));
            }
        }
    }
}
