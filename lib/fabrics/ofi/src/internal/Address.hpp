#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <sys/types.h>
#include "Endpoint.hpp"
#include "PassiveEndpoint.hpp"

namespace mxl::lib::fabrics::ofi
{
    class FabricAddress
    {
    public:
        explicit FabricAddress();

        static FabricAddress fromEndpoint(Endpoint&);
        static FabricAddress fromEndpoint(PassiveEndpoint&);

        friend std::ostream& operator<<(std::ostream&, FabricAddress const&);
        friend std::istream& operator>>(std::istream&, FabricAddress&);

        void* raw();

    private:
        explicit FabricAddress(std::vector<uint8_t> addr);
        static FabricAddress retrieveFabricAddress(::fid_t);

        std::vector<uint8_t> _inner;
    };
}
