#include "MemoryRegion.hpp"
#include <cstdint>
#include <memory>
#include <bits/types/struct_iovec.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include "Domain.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::shared_ptr<MemoryRegion> MemoryRegion::reg(std::shared_ptr<Domain> domain, Regions const& regions, uint64_t access)
    {
        if (regions.empty())
        {
            throw std::invalid_argument("Cannot register an empty vector of regions");
        }

        auto iovecs = regions.toIovec();

        ::fid_mr* raw;

        ::fi_mr_attr attr{};
        attr.mr_iov = iovecs.data();
        attr.iov_count = iovecs.size();
        attr.access = access;
        attr.offset = 0;             // reserved to 0
        attr.requested_key = 0;      // ignore if FI_MR_PROV_KEY mr mode is set
        attr.context = nullptr;      // not used
        attr.auth_key_size = 0;      // not used
        attr.auth_key = nullptr;     // not used
        attr.iface = FI_HMEM_SYSTEM; // this will change when we add HMEM support (also need to set attr.device bits)
        attr.hmem_data = nullptr;    // not used
        attr.page_size = 0;          // not used
        attr.base_mr = nullptr;      // not used
        attr.sub_mr_cnt = 0;         // not used

        fiCall(fi_mr_regattr,
            "Failed to register memory region",
            domain->raw(),
            &attr,
            FI_RMA_EVENT, // flags: this will need to be modified when we will add HMEM support
            &raw);

        struct MakeSharedEnabler : public MemoryRegion
        {
            MakeSharedEnabler(::fid_mr* raw, std::shared_ptr<Domain> domain)
                : MemoryRegion(raw, std::move(domain))
            {}
        };

        return std::make_shared<MakeSharedEnabler>(raw, std::move(domain));
    }

    MemoryRegion::~MemoryRegion()
    {
        close();
    }

    MemoryRegion::MemoryRegion(MemoryRegion&& other) noexcept
        : _raw(other._raw)
        , _domain(std::move(other._domain))
    {
        other._raw = nullptr;
    }

    MemoryRegion& MemoryRegion::operator=(MemoryRegion&& other) noexcept
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _domain = std::move(other._domain);

        return *this;
    }

    void* MemoryRegion::getLocalMemoryDescriptor() const noexcept
    {
        return fi_mr_desc(_raw);
    }

    uint64_t MemoryRegion::getRemoteKey() const noexcept
    {
        return fi_mr_key(_raw);
    }

    MemoryRegion::MemoryRegion(::fid_mr* raw, std::shared_ptr<Domain> domain)
        : _raw(raw)
        , _domain(std::move(domain))
    {}

    void MemoryRegion::close()
    {
        if (_raw)
        {
            fiCall(::fi_close, "Failed to close memory region", &_raw->fid);
            _raw = nullptr;
        }
    }
}
