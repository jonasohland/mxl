#include "PassiveEndpoint.hpp"
#include <memory>
#include <optional>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_endpoint.h>
#include "internal/Logging.hpp"
#include "EventQueue.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::shared_ptr<PassiveEndpoint> PassiveEndpoint::create(std::shared_ptr<Fabric> fabric)
    {
        ::fid_pep* pep;

        fiCall(::fi_passive_ep, "Failed to create passive endpoint", fabric->raw(), fabric->info().raw(), &pep, nullptr);

        struct MakeSharedEnabler : public PassiveEndpoint
        {
            MakeSharedEnabler(::fid_pep* raw, std::shared_ptr<Fabric> fabric, std::optional<std::shared_ptr<EventQueue>> eq)
                : PassiveEndpoint(raw, std::move(fabric), std::move(eq))
            {}
        };

        return std::make_shared<MakeSharedEnabler>(pep, fabric, std::nullopt);
    }

    PassiveEndpoint::~PassiveEndpoint()
    {
        close();
    }

    PassiveEndpoint::PassiveEndpoint(::fid_pep* raw, std::shared_ptr<Fabric> fabric, std::optional<std::shared_ptr<EventQueue>> eq)
        : _raw(raw)
        , _fabric(std::move(fabric))
        , _eq(eq)
    {}

    void PassiveEndpoint::close()
    {
        if (_raw)
        {
            fiCall(::fi_close, "Failed to close passive endpoint", &_raw->fid);
            _raw = nullptr;
        }
    }

    PassiveEndpoint::PassiveEndpoint(PassiveEndpoint&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    PassiveEndpoint& PassiveEndpoint::operator=(PassiveEndpoint&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _fabric = std::move(other._fabric);

        return *this;
    }

    void PassiveEndpoint::bind(std::shared_ptr<EventQueue> eq)
    {
        auto fid = eq->raw()->fid;
        fiCall(::fi_pep_bind, "Failed to bind event queue to passive endpoint", _raw, &fid, 0);

        _eq = eq;
    }

    void PassiveEndpoint::listen()
    {
        fiCall(::fi_listen, "Failed to transition the endpoint into listener mode", _raw);
    }

    void PassiveEndpoint::reject(ConnNotificationEntry const& entry)
    {
        if (!entry.getFid())
        {
            MXL_ERROR("Cannot reject a connection notification entry without a fid");
            return;
        }

        fiCall(::fi_reject, "Failed to reject connection request", _raw, *entry.getFid(), nullptr, 0);
    }

    std::shared_ptr<EventQueue> PassiveEndpoint::eventQueue() const
    {
        {
            if (!_eq)
            {
                throw std::runtime_error("Event queue is not bound to the endpoint"); // Is this the right throw??
            }

            return *_eq;
        }
    }

    ::fid_pep* PassiveEndpoint::raw() noexcept
    {
        return _raw;
    }

    ::fid_pep const* PassiveEndpoint::raw() const noexcept
    {
        return _raw;
    }

}