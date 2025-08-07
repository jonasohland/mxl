#include "Endpoint.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <uuid.h>
#include <bits/types/struct_iovec.h>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>
#include "internal/Logging.hpp"
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"
#include "FIInfo.hpp"
#include "LocalRegion.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    Endpoint Endpoint::create(std::shared_ptr<Domain> domain)
    {
        return Endpoint::create(std::move(domain), uuids::uuid_system_generator{}());
    }

    Endpoint Endpoint::create(std::shared_ptr<Domain> domain, FIInfoView info)
    {
        return Endpoint::create(std::move(domain), uuids::uuid_system_generator{}(), info);
    }

    Endpoint Endpoint::create(std::shared_ptr<Domain> domain, uuids::uuid epid, FIInfoView info)
    {
        ::fid_ep* raw;

        auto uuidMem = std::make_unique<uuids::uuid>(epid);
        fiCall(::fi_endpoint, "Failed to create endpoint", domain->raw(), info.raw(), &raw, uuidMem.get());

        uuidMem.release();

        // expose the private constructor to std::make_shared inside this function
        struct MakeSharedEnabler : public Endpoint
        {
            MakeSharedEnabler(::fid_ep* raw, FIInfoView info, std::shared_ptr<Domain> domain)
                : Endpoint(raw, info, domain)
            {}
        };

        return {raw, info, std::move(domain)};
    }

    Endpoint Endpoint::create(std::shared_ptr<Domain> domain, uuids::uuid epid)
    {
        return Endpoint::create(domain, epid, domain->fabric()->info());
    }

    uuids::uuid Endpoint::idFromFID(::fid_t fid) noexcept
    {
        return *reinterpret_cast<uuids::uuid*>(fid->context);
    }

    uuids::uuid Endpoint::idFromFID(::fid_ep* fid) noexcept
    {
        return Endpoint::idFromFID(&fid->fid);
    }

    uuids::uuid Endpoint::id() const noexcept
    {
        return Endpoint::idFromFID(&_raw->fid);
    }

    Endpoint::~Endpoint()
    {
        close();
    }

    Endpoint::Endpoint(::fid_ep* raw, FIInfoView info, std::shared_ptr<Domain> domain, std::optional<std::shared_ptr<CompletionQueue>> cq,
        std::optional<std::shared_ptr<EventQueue>> eq)
        : _raw(raw)
        , _info(info.owned())
        , _domain(std::move(domain))
        , _cq(std::move(cq))
        , _eq(std::move(eq))
    {
        MXL_INFO("Endpoint {} created", uuids::to_string(Endpoint::idFromFID(raw)));
    }

    void Endpoint::close()
    {
        if (_raw)
        {
            auto idp = reinterpret_cast<uuids::uuid*>(_raw->fid.context);
            auto id = *idp;
            delete idp;
            fiCall(::fi_close, "Failed to close endpoint", &_raw->fid);

            MXL_INFO("Endoint {} closed", uuids::to_string(id));

            _raw = nullptr;
        }
    }

    Endpoint::Endpoint(Endpoint&& other) noexcept
        : _raw(other._raw)
        , _info(std::move(other._info))
        , _domain(std::move(other._domain))
        , _cq(std::move(other._cq))
        , _eq(std::move(other._eq))
    {
        other._raw = nullptr;
    }

    Endpoint& Endpoint::operator=(Endpoint&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _info = std::move(other._info);
        _domain = std::move(other._domain);
        _eq = std::move(other._eq);
        _cq = std::move(other._cq);

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

    FabricAddress Endpoint::localAddress() const
    {
        return FabricAddress::fromFid(&_raw->fid);
    }

    std::shared_ptr<CompletionQueue> Endpoint::completionQueue() const
    {
        if (!_cq)
        {
            throw std::runtime_error("no Completion queue is bound to the endpoint"); // Is this the right throw??
        }

        return *_cq;
    }

    std::shared_ptr<EventQueue> Endpoint::eventQueue() const
    {
        if (!_eq)
        {
            throw std::runtime_error("No event queue bound to the endpoint"); // Is this the right throw??
        }

        return *_eq;
    }

    std::pair<std::optional<Completion>, std::optional<Event>> Endpoint::poll()
    {
        std::optional<Completion> completion{std::nullopt};
        std::optional<Event> event{std::nullopt};

        if (_cq)
        {
            completion = _cq.value()->read();
        }

        if (_eq)
        {
            event = _eq.value()->read();
        }

        return {completion, event};
    }

    std::shared_ptr<Domain> Endpoint::domain() const
    {
        return _domain;
    }

    FIInfoView Endpoint::info() const noexcept
    {
        return _info.view();
    }

    ::fid_ep* Endpoint::raw() noexcept
    {
        return _raw;
    }

    ::fid_ep const* Endpoint::raw() const noexcept
    {
        return _raw;
    }

    void Endpoint::write(LocalRegionGroup const& localGroup, RemoteRegionGroup const& remoteGroup, ::fi_addr_t destAddr,
        std::optional<uint64_t> immData)
    {
        uint64_t data = immData.value_or(0);
        uint64_t flags = FI_TRANSMIT_COMPLETE | FI_COMMIT_COMPLETE;
        flags |= immData.has_value() ? FI_REMOTE_CQ_DATA : 0;

        ::fi_msg_rma msg = {
            .msg_iov = localGroup.iovec(),
            .desc = const_cast<void**>(localGroup.desc()),
            .iov_count = localGroup.count(),
            .addr = destAddr,
            .rma_iov = remoteGroup.rmaIovs(),
            .rma_iov_count = remoteGroup.count(),
            .context = _raw,
            .data = data,
        };

        fiCall(::fi_writemsg, "Failed to push rma write to work queue.", _raw, &msg, flags);
    }

    void Endpoint::recv(LocalRegion region)
    {
        // TODO: this should use fi_recvmsg and fi_msg so we can also pass the local endpoint in the context
        auto iovec = region.toIov();
        fiCall(::fi_recv, "Failed to push recv to work queue", _raw, iovec.iov_base, iovec.iov_len, nullptr, FI_ADDR_UNSPEC, nullptr);
    }
}
