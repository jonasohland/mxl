#pragma once

#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>

namespace mxl::lib::fabrics
{
    class EndpointInfo
    {
    public:
        static EndpointInfo lookup();

        ~EndpointInfo();

    private:
        EndpointInfo(::fi_info* info);

        ::fi_info* _info;
    };

    class Endpoint
    {
    private:
        ::fid_ep _ep;
    };
}
