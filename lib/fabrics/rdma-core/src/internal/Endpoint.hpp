#pragma once

#include <rdma/rdma_cma.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "ConnectionManagement.hpp"
#include "LocalRegion.hpp"
#include "QueuePair.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class PassiveEndpoint;

    class ActiveEndpoint final
    {
    public:
        // Verbs ops
        void write(LocalRegion& localRegion, RemoteRegion& remoteRegion);
        void send(LocalRegion& localRegion);
        void recv(LocalRegion& localRegion);

        // Completions
        std::optional<Completion> readCq();
        std::optional<Completion> readCqBlocking();

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

    private:
        PassiveEndpoint(ConnectionManagement cm);

    private:
        friend class ActiveEndpoint;

    private:
        ConnectionManagement _cm;
    };
}
