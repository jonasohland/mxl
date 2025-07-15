#pragma once

#include <cstdint>
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Endpoint
    {
    public:
        ~Endpoint();
        Endpoint(Endpoint const&) = delete;
        void operator=(Endpoint const&) = delete;

        Endpoint(Endpoint&&) noexcept;
        Endpoint& operator=(Endpoint&&);

        static std::shared_ptr<Endpoint> create(std::shared_ptr<Domain> domain);

        void bind(EventQueue const& eq);
        void bind(CompletionQueue const& cq, uint64_t flags);

        void enable();

        void accept();
        void connect(void const* addr);
        void shutdown();

    private:
        void close();

        Endpoint(::fid_ep* raw, std::shared_ptr<Domain> domain);

        ::fid_ep* _raw;
        std::shared_ptr<Domain> _domain;
    };
}
