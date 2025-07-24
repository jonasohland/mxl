#include "Endpoint.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_rma.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"
#include "FIInfo.hpp"
#include "MemoryRegion.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::shared_ptr<Endpoint> Endpoint::create(std::shared_ptr<Domain> domain, std::optional<FIInfoView> remoteInfo)
    {
        ::fid_ep* raw;

        // If we are connecting to a remote endpoint, we need to use the remote info otherwise we create the endpoint with our local info.
        auto info = remoteInfo ? remoteInfo->raw() : domain->fabric()->info().raw();

        fiCall(::fi_endpoint, "Failed to create endpoint", domain->raw(), info, &raw, nullptr);

        // expose the private constructor to std::make_shared inside this function
        struct MakeSharedEnabler : public Endpoint
        {
            MakeSharedEnabler(::fid_ep* raw, std::shared_ptr<Domain> domain)
                : Endpoint(raw, domain)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(raw, std::move(domain));
    }

    Endpoint::~Endpoint()
    {
        close();
    }

    Endpoint::Endpoint(::fid_ep* raw, std::shared_ptr<Domain> domain, std::optional<std::shared_ptr<CompletionQueue>> cq,
        std::optional<std::shared_ptr<EventQueue>> eq)
        : _raw(raw)
        , _domain(std::move(domain))
        , _cq(std::move(cq))
        , _eq(std::move(eq))
    {}

    void Endpoint::close()
    {
        if (_raw)
        {
            fiCall(::fi_close, "Failed to close endpoint", &_raw->fid);
            _raw = nullptr;
        }
    }

    Endpoint::Endpoint(Endpoint&& other) noexcept
        : _raw(other._raw)
        , _domain(std::move(other._domain))
    {
        other._raw = nullptr;
    }

    Endpoint& Endpoint::operator=(Endpoint&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _domain = std::move(other._domain);

        return *this;
    }

    void Endpoint::bind(std::shared_ptr<EventQueue> eq)
    {
        fiCall(::fi_ep_bind, "Failed to bind event queue to endpoint", _raw, &eq->raw()->fid, 0);

        _eq = eq;
    }

    void Endpoint::bind(std::shared_ptr<CompletionQueue> cq, uint64_t flags)
    {
        fiCall(::fi_ep_bind, "Failed to bind completion queue to endpoint", _raw, &cq->raw()->fid, flags);

        _cq = cq;
    }

    void Endpoint::enable()
    {
        fiCall(::fi_enable, "Failed to enable endpoint", _raw);
    }

    void Endpoint::accept()
    {
        uint8_t dummy;
        fiCall(::fi_accept, "Failed to accept connection", _raw, &dummy, sizeof(dummy));
    }

    void Endpoint::connect(FabricAddress const& addr)
    {
        fiCall(::fi_connect, "Failed to connect endpoint", _raw, addr.raw(), nullptr, 0);
    }

    void Endpoint::shutdown()
    {
        fiCall(::fi_shutdown, "Failed to shutdown endpoint", _raw, 0);
    }

    std::shared_ptr<CompletionQueue> Endpoint::completionQueue() const
    {
        if (!_cq)
        {
            throw std::runtime_error("Completion queue is not bound to the endpoint"); // Is this the right throw??
        }

        return *_cq;
    }

    std::shared_ptr<EventQueue> Endpoint::eventQueue() const
    {
        {
            if (!_eq)
            {
                throw std::runtime_error("Event queue is not bound to the endpoint"); // Is this the right throw??
            }

            return *_eq;
        }
    }

    ::fid_ep* Endpoint::raw() noexcept
    {
        return _raw;
    }

    ::fid_ep const* Endpoint::raw() const noexcept
    {
        return _raw;
    }

    void Endpoint::write(RegisteredRegion& localRegion, RemoteRegion const& remoteRegion, ::fi_addr_t destAddr)
    {
        auto iovec = localRegion.region().to_iovec();
        auto desc = std::vector<void*>{localRegion.desc()};

        // TODO: catch error and convert to an internal error
        fiCall(::fi_writev,
            "Failed to push rma write to work queue.",
            _raw,
            iovec.data(),
            desc.data(),
            iovec.size(),
            destAddr,
            remoteRegion.addr,
            remoteRegion.rkey,
            nullptr);
    }

}
