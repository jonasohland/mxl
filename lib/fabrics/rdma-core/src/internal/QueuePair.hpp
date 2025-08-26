#pragma once

#include <infiniband/verbs.h>
#include "ProtectionDomain.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class ConnectionManagement;

    struct QueuePairAttr
    {
        ::ibv_qp_type qpType;

        static QueuePairAttr defaults() noexcept;

        [[nodiscard]]
        ::ibv_qp_init_attr toIbv() const noexcept;
    };

    class QueuePair
    {
    public:
        ~QueuePair();

        // Copy constructor is deleted
        QueuePair(QueuePair const&) = delete;
        void operator=(QueuePair const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        QueuePair(QueuePair&&) noexcept;
        // Move-assigning an endpoint to another releases all resources from the moved-into endpoint and
        QueuePair& operator=(QueuePair&&);

        ::ibv_qp* raw() noexcept;

    private:
        QueuePair(ProtectionDomain& pd, QueuePairAttr attr);
        void close();

    private:
        friend class ConnectionManagement;

    private:
        ::ibv_qp* _raw;
    };

}
