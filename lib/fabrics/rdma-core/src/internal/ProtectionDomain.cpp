#include "ProtectionDomain.hpp"
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include "internal/Logging.hpp"
#include "ConnectionManagement.hpp"
#include "RegisteredRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    ProtectionDomain::ProtectionDomain(ConnectionManagement& cm)
    {
        MXL_INFO("attempt to create PD");
        _raw = ibv_alloc_pd(cm.raw()->verbs);
        if (_raw == nullptr)
        {
            throw std::runtime_error(fmt::format("Failed to allocate protection domain: {}", strerror(errno)));
        }
        MXL_INFO("created pd");
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

    void ProtectionDomain::registerRegionGroups(RegionGroups const& regionGroups, uint64_t access)
    {
        std::ranges::transform(
            regionGroups.view(), std::back_inserter(_registeredRegionGroups), [&](auto const& group) { return registerRegionGroup(group, access); });
    }

    std::vector<LocalRegionGroup> ProtectionDomain::localRegionGroups() noexcept
    {
        return toLocal(_registeredRegionGroups);
    }

    std::vector<RemoteRegion> ProtectionDomain::remoteRegions() const noexcept
    {
        return toRemote(_registeredRegionGroups);
    }

    RegisteredRegion ProtectionDomain::registerRegion(Region const& region, uint64_t access)
    {
        return RegisteredRegion{MemoryRegion::reg(*this, region, access), region};
    }

    RegisteredRegionGroup ProtectionDomain::registerRegionGroup(RegionGroup const& regionGroup, uint64_t access)
    {
        std::vector<RegisteredRegion> out;
        std::ranges::transform(regionGroup.view(), std::back_inserter(out), [&](auto const& region) { return registerRegion(region, access); });
        return RegisteredRegionGroup{std::move(out)};
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
