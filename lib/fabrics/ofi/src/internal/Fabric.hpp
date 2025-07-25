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

        [[nodiscard]]
        std::shared_ptr<FIInfo> info() noexcept;

        static std::shared_ptr<Fabric> open(std::shared_ptr<FIInfo> info);

    private:
        void close();

        Fabric(::fid_fabric* raw, std::shared_ptr<FIInfo> info);

        ::fid_fabric* _raw;
        std::shared_ptr<FIInfo> _info;
    };

}
