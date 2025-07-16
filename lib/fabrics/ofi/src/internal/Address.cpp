#include "Address.hpp"
#include <cstdint>
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include "mxl/mxl.h"
#include "Endpoint.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    FabricAddress::FabricAddress()
        : _inner()
    {}

    FabricAddress::FabricAddress(std::vector<uint8_t> addr)
        : _inner(std::move(addr))
    {}

    std::shared_ptr<FabricAddress> FabricAddress::retrieveFabricAddress(::fid_t fid)
    {
        // First obtain the address length
        size_t addrlen = 0;
        auto ret = fi_getname(fid, nullptr, &addrlen);
        if (ret != -FI_ETOOSMALL)
        {
            throw FIException("Failed to get address length from endpoint.", MXL_ERR_UNKNOWN, ret);
        }

        // Now that we have the address length, allocate a receiving buffer and call fi_getname again to retrieve the actual address
        std::vector<uint8_t> addr(addrlen);
        fiCall(fi_getname, "Failed to retrieve endpoint's local address.", fid, addr.data(), &addrlen);

        struct MakeSharedEnabler : public FabricAddress
        {
            MakeSharedEnabler(std::vector<uint8_t> addr)
                : FabricAddress(std::move(addr))
            {}
        };

        return std::make_shared<MakeSharedEnabler>(std::move(addr));
    }

    std::shared_ptr<FabricAddress> FabricAddress::fromEndpoint(Endpoint& ep)
    {
        return retrieveFabricAddress(&ep.raw()->fid);
    }

    std::shared_ptr<FabricAddress> FabricAddress::fromEndpoint(PassiveEndpoint& pep)
    {
        return retrieveFabricAddress(&pep.raw()->fid);
    }

    void* FabricAddress::raw()
    {
        return _inner.data();
    }

    std::ostream& operator<<(std::ostream& os, FabricAddress const& addr)
    {
        os.write(reinterpret_cast<char const*>(addr._inner.data()), addr._inner.size());
        return os;
    }

    std::istream& operator>>(std::istream& is, FabricAddress& addr)
    {
        addr._inner.clear();

        uint8_t raw;
        while (is >> raw)
        {
            addr._inner.push_back(raw);
        }
        return is;
    }
}
