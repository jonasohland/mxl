#pragma once

#include <optional>
#include <rdma/rdma_cma.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "ProtectionDomain.hpp"
#include "QueuePair.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class ConnectionManagement final
    {
    public:
        static ConnectionManagement create();
        ~ConnectionManagement();

        // Copy constructor is deleted
        ConnectionManagement(ConnectionManagement const&) = delete;
        void operator=(ConnectionManagement const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        ConnectionManagement(ConnectionManagement&&) noexcept;
        // Move-assigning an endpoint to another releases all resources from the moved-into endpoint and
        ConnectionManagement& operator=(ConnectionManagement&&);

        void bind(Address& srcAddr);
        void listen();
        ConnectionManagement waitConnectionRequest();
        void connect(Address& dstAddr);

        void createProtectionDomain() noexcept;
        void createCompletionQueue() noexcept;
        void createQueuePair(QueuePairAttr attr);

        ProtectionDomain& protectionDomain();
        CompletionQueue& completionQueue();
        QueuePair& queuePair();

        ::rdma_cm_id* raw() noexcept;

    private:
        ConnectionManagement(::rdma_cm_id*);

        void close();

    private:
        ::rdma_cm_id* _raw;
        std::optional<AddressInfo> _srcAddr;

        std::optional<ProtectionDomain> _pd;
        std::optional<CompletionQueue> _cq;
        std::optional<QueuePair> _qp;
    };
}
