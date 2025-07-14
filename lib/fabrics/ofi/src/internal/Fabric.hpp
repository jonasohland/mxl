#pragma once

#include <memory>
#include <rdma/fabric.h>
#include "FIInfo.hpp"

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

        ::fid_fabric* raw() noexcept;
        [[nodiscard]]
        ::fid_fabric const* raw() const noexcept;

        static std::shared_ptr<Fabric> open(FIInfoView info);

    private:
        void close();

        Fabric(::fid_fabric* raw);
        ::fid_fabric* _raw;
    };

}
