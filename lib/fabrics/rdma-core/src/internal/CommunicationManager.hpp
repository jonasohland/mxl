#pragma once

#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    class CommunicationManager
    {
    public:

    private:
        ::rdma_cm_id* _id;
    };
}
