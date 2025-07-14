#pragma once

#include <memory>
#include <rdma/fi_domain.h>
#include "Fabric.hpp"
#include "FIInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Domain
    {
    public:
        ~Domain();

        Domain(Domain const&) = delete;
        void operator=(Domain const&) = delete;

        Domain(Domain&&) noexcept;
        Domain& operator=(Domain&&);

        ::fid_domain* raw() noexcept;
        [[nodiscard]]
        ::fid_domain const* raw() const noexcept;

        static std::shared_ptr<Domain> open(FIInfoView info, std::shared_ptr<Fabric> fabric);

    private:
        void close();

        Domain(::fid_domain*, FIInfo, std::shared_ptr<Fabric>);

        ::fid_domain* _raw;
        FIInfo _sourceInfo;
        std::shared_ptr<Fabric> _fabric;
    };
}
