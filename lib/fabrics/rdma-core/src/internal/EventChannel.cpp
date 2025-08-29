#include "EventChannel.hpp"
#include <memory>
#include <rdma/rdma_cma.h>
#include "Exception.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    using Event = EventChannel::Event;

    Event::Event(::rdma_cm_event* event)
        : _raw(event)
    {}

    Event::~Event()
    {
        if (_raw)
        {
            ::rdma_ack_cm_event(_raw);
        }
    }

    bool Event::isSuccess() const noexcept
    {
        return _raw->status == 0;
    }

    bool Event::isAddrResolved() const noexcept
    {
        return _raw->event == RDMA_CM_EVENT_ADDR_RESOLVED;
    }

    bool Event::isRouteResolved() const noexcept
    {
        return _raw->event == RDMA_CM_EVENT_ROUTE_RESOLVED;
    }

    bool Event::isConnectionRequest() const noexcept
    {
        return _raw->event == RDMA_CM_EVENT_CONNECT_REQUEST;
    }

    bool Event::isConnectionEstablished() const noexcept
    {
        return _raw->event == RDMA_CM_EVENT_ESTABLISHED;
    }

    std::string Event::toString() const noexcept
    {
        return rdma_event_str(_raw->event);
    }

    ::rdma_cm_id* Event::clientId() noexcept
    {
        return _raw->id;
    }

    ::rdma_cm_id* Event::listenId() noexcept
    {
        return _raw->listen_id;
    }

    std::shared_ptr<EventChannel> EventChannel::create()
    {
        auto raw = rdma_create_event_channel();
        if (!raw)
        {
            throw Exception::internal("Failed to create event channel");
        }

        struct MakeSharedEnabler : public EventChannel
        {
            MakeSharedEnabler(::rdma_event_channel* raw)
                : EventChannel(raw)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(raw);
    }

    EventChannel::~EventChannel()
    {
        close();
    }

    Event EventChannel::get()
    {
        ::rdma_cm_event* event;

        if (rdma_get_cm_event(_raw, &event))
        {
            throw Exception::internal("Faield to get CM event.");
        }

        return {event};
    }

    ::rdma_event_channel* EventChannel::raw() noexcept
    {
        return _raw;
    }

    EventChannel::EventChannel(::rdma_event_channel* ec)
        : _raw(ec)
    {}

    void EventChannel::close()
    {
        if (_raw)
        {
            rdma_destroy_event_channel(_raw);

            _raw = nullptr;
        }
    }

}
