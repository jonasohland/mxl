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
        ConnectionManagement waitConnectionRequest(std::chrono::steady_clock::duration timeout);
        void accept();

        ProtectionDomain& pd();
        CompletionQueue& cq();

        // Client Connection ops
        void resolveAddr(Address& dstAddr, std::chrono::steady_clock::duration timeout);
        void resolveRoute(std::chrono::steady_clock::duration timeout);
        void createProtectionDomain() noexcept;
        void createCompletionQueue() noexcept;
        void createQueuePair(QueuePairAttr attr);
        void connect();
        void disconnect();

        // Client Verbs ops
        void write(std::uint64_t id, LocalRegion& localRegion, RemoteRegion& remoteRegion);
        void recv(LocalRegion& localRegion);

        // Client Completions
        std::optional<Completion> readCq();
        std::optional<Completion> readCqBlocking(std::chrono::steady_clock::duration timeout);

        std::optional<EventChannel::Event> readEvent();
        std::optional<EventChannel::Event> readEventBlocking(std::chrono::steady_clock::duration timeout);

        ::rdma_cm_id* raw() noexcept;

    private:
        ConnectionManagement(::rdma_cm_id*, std::shared_ptr<EventChannel>, std::optional<ProtectionDomain> = std::nullopt,
            std::optional<CompletionQueue> = std::nullopt, bool = false);

        void close();

    private:
        ::rdma_cm_id* _raw;
        std::shared_ptr<EventChannel> _ec;

        std::optional<ProtectionDomain> _pd;
        std::optional<CompletionQueue> _cq;
        bool _hasQp = false;
    };
}
