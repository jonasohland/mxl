#pragma once

#include <memory>
#include <rdma/fabric.h>

namespace mxl::lib::fabrics::ofi
{
    class Fabric
    {
    public:
        ~Fabric();

        Fabric(Fabric const&) = delete;
        void operator=(Fabric const&) = delete;

        Fabric(Fabric&&) noexcept;
        Fabric& operator=(Fabric&&);

        static std::shared_ptr<Fabric> open(::fi_fabric_attr* attr);

    private:
        void close();

        Fabric(::fid_fabric* raw);
        ::fid_fabric* _raw;
    };

}
