#pragma once

#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include "Domain.hpp"
#include "Fabric.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Endpoint
    {
    public:
        std::shared_ptr<Endpoint> bind(std::shared_ptr<Domain>, std::shared_ptr<Fabric>);
        std::shared_ptr<Endpoint> connect(std::shared_ptr<Domain>, std::shared_ptr<Fabric>);

    private:
        Endpoint(::fid_ep*, std::shared_ptr<Domain>, std::shared_ptr<Fabric>);

        ::fid_ep _ep;
        std::shared_ptr<Domain> _domain;
        std::shared_ptr<Fabric> _fabric;
    };
}
