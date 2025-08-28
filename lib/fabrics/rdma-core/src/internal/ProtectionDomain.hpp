#pragma once

#include <cstdint>
#include <vector>
#include <infiniband/verbs.h>
#include "LocalRegion.hpp"
#include "Region.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    class ConnectionManagement;
    class RegisteredRegionGroup;
    class RegisteredRegion;

    class ProtectionDomain
    {
    public:
        ~ProtectionDomain(); // Copy constructor is deleted

        ProtectionDomain(ProtectionDomain const&) = delete;
        void operator=(ProtectionDomain const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        ProtectionDomain(ProtectionDomain&&) noexcept;
        // Move-assigning an endpoint to another releases all resources from the moved-into endpoint and
        ProtectionDomain& operator=(ProtectionDomain&&);
        /// Register a list of memory region group to this domain. The domain will own the registered groups.
        void registerRegionGroups(RegionGroups const& regionGroups, uint64_t access);
        RegisteredRegion registerRegion(Region const& region, uint64_t access);
        RegisteredRegionGroup registerRegionGroup(RegionGroup const& regionGroup, uint64_t access);
        /// Get the local groups associated with the registered groups to this domain.
        [[nodiscard]]
        std::vector<LocalRegionGroup> localRegionGroups() noexcept;

        /// Get the remote groups associated with the registered groups to this domain.
        [[nodiscard]]
        std::vector<RemoteRegion> remoteRegions() const noexcept;

        ::ibv_pd* raw() noexcept;

    private:
        ProtectionDomain(ConnectionManagement& cm);

        void close();

    private:
        friend class ConnectionManagement;

    private:
        ibv_pd* _raw;
        std::vector<RegisteredRegionGroup> _registeredRegionGroups;
    };
}
