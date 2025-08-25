#include "Address.hpp"
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    Address::Address(std::string node, std::string service)
    {
        ::rdma_addrinfo hints = {};

        hints.ai_flags = RAI_PASSIVE;
        hints.ai_port_space = RDMA_PS_TCP;

        int ret = rdma_getaddrinfo(node.c_str(), service.c_str(), &hints, &_ai);
        if (ret != 0)
        {
            throw std::runtime_error(fmt::format("failed to get addrinfo for {}:{} reason: {}", node, service, strerror(errno)));
        }
    }

    Address::~Address()
    {
        rdma_freeaddrinfo(_ai);
    }

}
