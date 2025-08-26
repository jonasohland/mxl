#pragma once

#include <string>
#include <rdma/rdma_cma.h>
#include <sys/socket.h>

namespace mxl::lib::fabrics::rdma_core
{
    class AddressInfo final
    {
    public:
        AddressInfo(::rdma_addrinfo*);
        ~AddressInfo();

        ::rdma_addrinfo* raw() noexcept;

    private:
        ::rdma_addrinfo* _raw;
    };

    class Address final
    {
    public:
        Address(std::string const& node, std::string const& service);

        ~Address();

        [[nodiscard]]
        AddressInfo aiPassive() const;
        [[nodiscard]]
        AddressInfo aiActive() const;

    private:
        Address(::rdma_addrinfo*);

    private:
        std::string _node;
        std::string _service;
    };

}
