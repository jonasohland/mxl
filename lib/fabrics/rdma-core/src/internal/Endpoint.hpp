#pragma once

#include "ConnectionManagement.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class PassiveEndpoint;

    class ActiveEndpoint final
    {
    public:
        // Verbs ops
        void write(std::uint64_t id, LocalRegion& localRegion, RemoteRegion& remoteRegion);

        // Completions
        std::optional<Completion> readCq();
        std::optional<Completion> readCqBlocking();

        ConnectionManagement& connectionManagement() noexcept;

    private:
        ActiveEndpoint(ConnectionManagement cm);
        ActiveEndpoint(PassiveEndpoint&& pep);

        void close();

    private:
        friend class PassiveEndpoint;

    private:
        ConnectionManagement _cm;
    };

    class PassiveEndpoint final
    {
    public:
        static PassiveEndpoint create(Address& bindAddr, QueuePairAttr qpAttr = QueuePairAttr::defaults());

        void listen();
        ActiveEndpoint waitConnectionRequest();
        ActiveEndpoint connect(Address& dstAddr);

        ConnectionManagement& connectionManagement() noexcept;

    private:
        PassiveEndpoint(ConnectionManagement cm);

    private:
        friend class ActiveEndpoint;

    private:
        ConnectionManagement _cm;
    };
}
