#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <sys/types.h>

namespace mxl::lib::fabrics::ofi
{
    class FabricAddress
    {
    public:
        explicit FabricAddress();

        static FabricAddress fromFid(::fid_t fid)
        {
            return retrieveFabricAddress(fid);
        }

        friend std::ostream& operator<<(std::ostream&, FabricAddress const&);
        friend std::istream& operator>>(std::istream&, FabricAddress&);

        void* raw();
        
        [[nodiscard]]
        void* raw() const;

    private:
        explicit FabricAddress(std::vector<uint8_t> addr);
        static FabricAddress retrieveFabricAddress(::fid_t);

        std::vector<uint8_t> _inner;
    };
}
