#include "EventChannel.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <fcntl.h>
#include <rdma/rdma_cma.h>
#include <sys/epoll.h>
#include "Exception.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    using Event = EventChannel::Event;

    Event::Event(::rdma_cm_event* event)
        : _raw(event)
    {}

    Event::~Event()
    {
        close();
    }

    Event::Event(Event&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    Event& Event::operator=(Event&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
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

    bool Event::isDisconnected() const noexcept
    {
        return _raw->event == RDMA_CM_EVENT_DISCONNECTED;
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

    void Event::close()
    {
        if (_raw)
        {
            ::rdma_ack_cm_event(_raw);

            _raw = nullptr;
        }
    }

    std::shared_ptr<EventChannel> EventChannel::create()
    {
        auto raw = rdma_create_event_channel();

        if (!raw)
        {
            throw Exception::internal("Failed to create event channel");
        }

        // By default events are blocking, let's change that to non-blocking
        int flags = ::fcntl(raw->fd, F_GETFL);
        rdmaCall(::fcntl, "Failed to set completion channel as non-blocking", raw->fd, F_SETFL, flags | O_NONBLOCK);

        auto epollFd = epoll_create(1);
        if (epollFd == -1)
        {
            throw Exception::internal("Failed to create epoll file descriptor: {}", strerror(errno));
        }

        // Register completion channel file descriptor to epoll
        ::epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = raw->fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, raw->fd, &ev) == -1)
        {
            throw Exception::internal("Failed to register event channel file descriptor to epoll: {}", strerror(errno));
        }

        struct MakeSharedEnabler : public EventChannel
        {
            MakeSharedEnabler(::rdma_event_channel* raw, int epollFd)
                : EventChannel(raw, epollFd)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(raw, epollFd);
    }

    EventChannel::~EventChannel()
    {
        close();
    }

    std::optional<Event> EventChannel::get(std::chrono::milliseconds timeout)
    {
        ::epoll_event events;
        auto ret = epoll_wait(_epollFd, &events, 1, timeout.count());
        if (ret == -1)
        {
            throw Exception::internal("Failed to wait with epoll: {}", strerror(errno));
        }

        if (ret == 1)
        {
            ::rdma_cm_event* event;
            rdmaCall(rdma_get_cm_event, "Failed to get CM Event", _raw, &event);
            return {event};
        }

        return std::nullopt;
    }

    ::rdma_event_channel* EventChannel::raw() noexcept
    {
        return _raw;
    }

    EventChannel::EventChannel(::rdma_event_channel* ec, int pollFd)
        : _raw(ec)
        , _epollFd(pollFd)
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
