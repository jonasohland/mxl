#pragma once

#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include "EventQueue.hpp"
#include "EventQueueEntry.hpp"
#include "Fabric.hpp"

namespace mxl::lib::fabrics::ofi
{
    class PassiveEndpoint
    {
    public:
        ~PassiveEndpoint();

        PassiveEndpoint(PassiveEndpoint const&) = delete;
        void operator=(PassiveEndpoint const&) = delete;

        PassiveEndpoint(PassiveEndpoint&&) noexcept;
        PassiveEndpoint& operator=(PassiveEndpoint&&);

        static std::shared_ptr<PassiveEndpoint> create(std::shared_ptr<Fabric>);

        void bind(EventQueue const& eq);

        void listen();
        void reject(ConnNotificationEntry const& entry);

    private:
        void close();

        PassiveEndpoint(::fid_pep* raw, std::shared_ptr<Fabric> fabric);

        ::fid_pep* _raw;
        std::shared_ptr<Fabric> _fabric;
    };
}