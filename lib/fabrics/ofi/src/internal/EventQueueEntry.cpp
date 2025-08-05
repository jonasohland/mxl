#include "EventQueueEntry.hpp"
#include "Exception.hpp"
#include "FIInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    Event::ConnectionRequested::ConnectionRequested(::fid_t fid, FIInfo info)
        : _fid(fid)
        , _info(std::move(info))
    {}

    fid_t Event::ConnectionRequested::fid() const noexcept
    {
        return _fid;
    }

    FIInfoView Event::ConnectionRequested::info() const noexcept
    {
        return _info.view();
    }

    Event::Connected::Connected(::fid_t fid)
        : _fid(fid)
    {}

    ::fid_t Event::Connected::fid() const noexcept
    {
        return _fid;
    }

    Event::Shutdown::Shutdown(::fid_t fid)
        : _fid(fid)
    {}

    ::fid_t Event::Shutdown::fid() const noexcept
    {
        return _fid;
    }

    Event::Event(Inner ev)
        : _event(std::move(ev))
    {}

    Event Event::fromRaw(::fi_eq_cm_entry const* entry, uint32_t eventType)
    {
        // clang-format off
        switch (eventType)
        {
            case FI_CONNREQ:   return {ConnectionRequested{entry->fid, FIInfo::own(entry->info)}};
            case FI_CONNECTED: return {Connected{entry->fid}};
            case FI_SHUTDOWN:  return {Shutdown{entry->fid}};
            default:           throw Exception::internal("Unsupported event type returned from queue");
        }
        // clang-format on
    }

    bool Event::isConnReq() const noexcept
    {
        return std::holds_alternative<ConnectionRequested>(_event);
    }

    bool Event::isConnected() const noexcept
    {
        return std::holds_alternative<Connected>(_event);
    }

    bool Event::isShutdown() const noexcept
    {
        return std::holds_alternative<Shutdown>(_event);
    }

    FIInfoView Event::info() const
    {
        if (auto connReq = std::get_if<ConnectionRequested>(&_event); connReq != nullptr)
        {
            return connReq->info();
        }

        throw Exception::invalidState("Tried to access fi_info from an event that is not a connection request");
    }

    fid_t Event::fid() noexcept
    {
        return std::visit(overloaded{[](ConnectionRequested& ev) { return ev.fid(); },
                              [](Connected& ev) { return ev.fid(); },
                              [](Shutdown& ev)
                              {
                                  return ev.fid();
                              }},
            _event);
    }

}
