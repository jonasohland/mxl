#include "Endpoint.hpp"
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

namespace mxl::lib::fabrics::rdma_core
{
    Endpoint Endpoint::create()
    {}

    Endpoint::Endpoint(::rdma_cm_id* id)
        : _id(id)
    {}

}
