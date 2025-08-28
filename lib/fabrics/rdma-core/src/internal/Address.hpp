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

        AddressInfo(AddressInfo const&) = delete;
        void operator=(AddressInfo const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        AddressInfo(AddressInfo&&) noexcept;
        // Move-assigning an endpoint to another releases all resources from the moved-into endpoint and
        AddressInfo& operator=(AddressInfo&&);

        ::rdma_addrinfo* raw() noexcept;

    private:
        void close();

    private:
        ::rdma_addrinfo* _raw;
    };

    class Address final
    {
    public:
        Address(std::string const& node, std::string const& service);

        [[nodiscard]]
        AddressInfo aiPassive() const;
        [[nodiscard]]
        AddressInfo aiActive() const;

        [[nodiscard]]
        std::string toString() const noexcept;
        static Address fromString(std::string const& s);

    private:
        Address(::rdma_addrinfo*);

    private:
        std::string _node;
        std::string _service;
    };

}
