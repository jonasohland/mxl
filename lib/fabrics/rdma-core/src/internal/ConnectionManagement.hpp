#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <rdma/rdma_cma.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "EventChannel.hpp"
#include "ProtectionDomain.hpp"
#include "QueuePair.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class ConnectionManagement
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

        // Server ops
        void bind(Address& srcAddr);
        void listen();
        ConnectionManagement waitConnectionRequest(size_t retries = 15);
        void accept();

        ProtectionDomain& pd();
        CompletionQueue& cq();

        // Client Connection ops
        void resolveAddr(Address& dstAddr, std::chrono::milliseconds timeout);
        void resolveRoute(std::chrono::milliseconds timeout);
        void createProtectionDomain() noexcept;
        void createCompletionQueue() noexcept;
        void createQueuePair(QueuePairAttr attr);
        void connect();

        // Client Verbs ops
        void write(std::uint64_t id, LocalRegion& localRegion, RemoteRegion& remoteRegion);
        void recv(LocalRegion& localRegion);

        // Client Completions
        std::optional<Completion> readCq();
        std::optional<Completion> readCqBlocking();

        ::rdma_cm_id* raw() noexcept;

    private:
        ConnectionManagement(::rdma_cm_id*, std::shared_ptr<EventChannel>, std::optional<ProtectionDomain> = std::nullopt,
            std::optional<CompletionQueue> = std::nullopt);

        void close();

    private:
        ::rdma_cm_id* _raw;
        std::shared_ptr<EventChannel> _ec;

        std::optional<ProtectionDomain> _pd;
        std::optional<CompletionQueue> _cq;
    };
}
