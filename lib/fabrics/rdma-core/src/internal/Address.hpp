#pragma once

#include <string>
#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    class Address final
    {
    public:
        Address(std::string name, std::string port);
        ~Address();

        ::rdma_addrinfo* raw() noexcept;
        [[nodiscard]]
        ::rdma_addrinfo const* raw() const noexcept;

    private:
        ::rdma_addrinfo* _ai;
    };
}
