#pragma once

#include <vector>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>

namespace mxl::lib::fabrics
{
    class FabricAddress
    {
    public:
        FabricAddress();

        ~FabricAddress();

    private:
        std::vector<uint8_t> _addr;
    };
}
