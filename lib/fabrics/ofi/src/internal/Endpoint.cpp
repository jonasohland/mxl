#include "Endpoint.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::shared_ptr<Endpoint> Endpoint::create(std::shared_ptr<Domain> domain)
    {
        ::fid_ep* raw;

        fiCall(::fi_endpoint, "Failed to create endpoint", domain->raw(), domain->fabric()->info().raw(), &raw, nullptr);

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
        auto fid = eq->raw()->fid;

        fiCall(::fi_ep_bind, "Failed to bind event queue to endpoint", _raw, &fid, 0);

        _eq = eq;
    }

    void Endpoint::bind(std::shared_ptr<CompletionQueue> cq, uint64_t flags)
    {
        auto fid = cq->raw()->fid;

        fiCall(::fi_ep_bind, "Failed to bind completion queue to endpoint", _raw, &fid, flags);

        _cq = cq;
    }

    void Endpoint::enable()
    {
        fiCall(::fi_enable, "Failed to enable endpoint", _raw);
    }

    void Endpoint::accept()
    {
        fiCall(::fi_accept, "Failed to accept connection", _raw, nullptr, 0);
    }

    void Endpoint::connect(void const* addr)
    {
        fiCall(::fi_connect, "Failed to connect endpoint", _raw, addr, nullptr, 0);
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

}
