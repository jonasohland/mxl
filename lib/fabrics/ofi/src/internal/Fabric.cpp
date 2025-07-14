#include "Fabric.hpp"
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::shared_ptr<Fabric> Fabric::open(FIInfoView info)
    {
        ::fid_fabric* fid;

        fiCall(::fi_fabric2, "Failed to open fabric", info.raw(), &fid, 0, nullptr);

        // expose the private constructor to std::make_shared inside this function
        struct MakeSharedEnabler : public Fabric
        {
            MakeSharedEnabler(::fid_fabric* raw)
                : Fabric(raw)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(fid);
    }

    Fabric::Fabric(::fid_fabric* raw)
        : _raw(raw)
    {}

    Fabric::~Fabric()
    {
        close();
    }

    Fabric::Fabric(Fabric&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    Fabric& Fabric::operator=(Fabric&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    ::fid_fabric* Fabric::raw() noexcept
    {
        return _raw;
    }

    ::fid_fabric const* Fabric::raw() const noexcept
    {
        return _raw;
    }

    void Fabric::close()
    {
        if (_raw != nullptr)
        {
            fiCall(::fi_close, "failed to close fabric", &_raw->fid);
        }
    }

}
