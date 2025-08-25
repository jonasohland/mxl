#pragma once

#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    class Endpoint
    {
    public:
        static Endpoint create();

        ::rdma_cm_id* raw() noexcept;
        [[nodiscard]]
        ::rdma_cm_id const* raw() const noexcept;

    private:
        Endpoint(::rdma_cm_id* id);

    private:
        ::rdma_cm_id* _id;
    };
}
