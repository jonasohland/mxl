#pragma once

#include <memory>
#include <rdma/fi_domain.h>
#include "Fabric.hpp"

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

        static std::shared_ptr<Domain> open(std::shared_ptr<Fabric> fabric);

        [[nodiscard]]
        std::shared_ptr<Fabric> fabric() const noexcept
        {
            return _fabric;
        }

    private:
        void close();

        Domain(::fid_domain*, std::shared_ptr<Fabric>);

        ::fid_domain* _raw;
        std::shared_ptr<Fabric> _fabric;
    };
}
