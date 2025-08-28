#include "Address.hpp"
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    AddressInfo::AddressInfo(::rdma_addrinfo* ai)
        : _raw(ai)
    {}

    AddressInfo::~AddressInfo()
    {
        close();
    }

    AddressInfo::AddressInfo(AddressInfo&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    AddressInfo& AddressInfo::operator=(AddressInfo&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    ::rdma_addrinfo* AddressInfo::raw() noexcept
    {
        return _raw;
    }

    void AddressInfo::close()
    {
        if (_raw)
        {
            rdma_freeaddrinfo(_raw);
        }
    }

    Address::Address(std::string const& node, std::string const& service)
        : _node(std::move(node))
        , _service(std::move(service))
    {}

    AddressInfo Address::aiPassive() const
    {
        ::rdma_addrinfo hints = {}, *ai;

        hints.ai_flags = RAI_PASSIVE;
        hints.ai_port_space = RDMA_PS_TCP;

        int ret = rdma_getaddrinfo(_node.c_str(), _service.c_str(), &hints, &ai);
        if (ret != 0)
        {
            throw std::runtime_error(fmt::format("failed to get addrinfo for {}:{} reason: {}", _node, _service, strerror(errno)));
        }

        return {ai};
    }

    AddressInfo Address::aiActive() const
    {
        ::rdma_addrinfo hints = {}, *ai;

        hints.ai_port_space = RDMA_PS_TCP;

        int ret = rdma_getaddrinfo(_node.c_str(), _service.c_str(), &hints, &ai);
        if (ret != 0)
        {
            throw std::runtime_error(fmt::format("failed to get addrinfo for {}:{} reason: {}", _node, _service, strerror(errno)));
        }

        return {ai};
    }

    std::string Address::toString() const noexcept
    {
        return fmt::format("{}:{}", _node, _service);
    }

    Address Address::fromString(std::string const& s)
    {
        auto pos = s.find(":");
        if (pos > s.size())
        {
            throw std::runtime_error(
                fmt::format("Failed to convert string \"{}\" into Address, no ':' found. Expecting a string of format <node>:<service> ", s));
        }

        auto node = s.substr(0, pos);
        auto service = s.substr(pos + 1, s.size());

        return {node, service};
    }
}
