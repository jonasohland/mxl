#include "CompletionChannel.hpp"
#include <chrono>
#include <cstring>
#include <optional>
#include <fcntl.h>
#include <infiniband/verbs.h>
#include <sys/epoll.h>
#include "CompletionQueue.hpp"
#include "ConnectionManagement.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    using Event = CompletionChannel::Event;

    Event::Event(ibv_cq* cq)
        : _cq(cq)
    {}

    Event::~Event()
    {
        ibv_ack_cq_events(_cq, 1);
    }

    CompletionChannel CompletionChannel::create(ConnectionManagement& cm)
    {
        auto raw = ibv_create_comp_channel(cm.raw()->verbs);

        // By default events are blocking, let's change that to non-blocking
        int flags = fcntl(raw->fd, F_GETFL);
        rdmaCall(fcntl, "Failed to set completion channel as non-blocking", raw->fd, F_SETFL, flags | O_NONBLOCK);

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
            throw Exception::internal("Failed to register completion channel file descriptor to epoll: {}", strerror(errno));
        }

        return {raw, epollFd};
    }

    CompletionChannel::~CompletionChannel()
    {
        close();
    }

    CompletionChannel::CompletionChannel(CompletionChannel&& other) noexcept
        : _raw(other._raw)
        , _epollFd(other._epollFd)
    {
        other._raw = nullptr;
        other._epollFd = 0;
    }

    CompletionChannel& CompletionChannel::operator=(CompletionChannel&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _epollFd = other._epollFd;
        other._epollFd = 0;

        return *this;
    }

    std::optional<Event> CompletionChannel::get(ibv_cq* cq, std::chrono::milliseconds timeout)
    {
        void* ctx = nullptr;

        ::epoll_event events;
        auto ret = epoll_wait(_epollFd, &events, 1, timeout.count());
        if (ret == -1)
        {
            throw Exception::internal("Failed to wait with epoll: {}", strerror(errno));
        }
        if (ret == 1)
        {
            if (auto ret = ::ibv_get_cq_event(_raw, &cq, &ctx); ret == 0)
            {
                rdmaCall(ibv_req_notify_cq, "Failed to request cq notify", cq, 0);
                return {Event{cq}};
            }
        }

        return std::nullopt;
    }

    ::ibv_comp_channel* CompletionChannel::raw() noexcept
    {
        return _raw;
    }

    CompletionChannel::CompletionChannel(::ibv_comp_channel* raw, int epollFd)
        : _raw(raw)
        , _epollFd(epollFd)
    {}

    void CompletionChannel::close()
    {
        if (_epollFd != 0)
        {
            if (::close(_epollFd) == -1)
            {
                throw Exception::internal("Failed to close epoll file descriptor: {}", strerror(errno));
            }
        }

        if (_raw)
        {
            rdmaCall(ibv_destroy_comp_channel, "Failed to destroy completion channel", _raw);
        }
    }

}
