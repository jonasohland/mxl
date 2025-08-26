#pragma once

#include <infiniband/verbs.h>

namespace mxl::lib::fabrics::rdma_core
{

    class ConnectionManagement;

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

        ::ibv_pd* raw() noexcept;

    private:
        ProtectionDomain(ConnectionManagement& cm);

        void close();

    private:
        friend class ConnectionManagement;

    private:
        ibv_pd* _raw;
    };
}
