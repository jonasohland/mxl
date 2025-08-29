#pragma once

#include <chrono>
#include <memory>
#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    class EventChannel
    {
    public:
        class Event
        {
        public:
            Event(::rdma_cm_event*);
            ~Event();

            Event(Event const&) = delete;
            void operator=(Event const&) = delete;

            Event(Event&&) noexcept;
            Event& operator=(Event&&);

            [[nodiscard]]
            bool isSuccess() const noexcept;

            [[nodiscard]]
            bool isAddrResolved() const noexcept;
            [[nodiscard]]
            bool isRouteResolved() const noexcept;
            [[nodiscard]]
            bool isConnectionRequest() const noexcept;
            [[nodiscard]]
            bool isConnectionResponse() const noexcept;
            [[nodiscard]]
            bool isConnectionEstablished() const noexcept;
            [[nodiscard]]
            bool isDisconnected() const noexcept;

            [[nodiscard]]
            std::string toString() const noexcept;

            ::rdma_cm_id* clientId() noexcept;
            ::rdma_cm_id* listenId() noexcept;

        private:
            void close();

        private:
            ::rdma_cm_event* _raw;
        };

    public:
        static std::shared_ptr<EventChannel> create();

        ~EventChannel();
        // No copying, no moving
        EventChannel(EventChannel const&) = delete;
        void operator=(EventChannel const&) = delete;
        EventChannel(EventChannel&&) = delete;
        EventChannel& operator=(EventChannel&&) = delete;

        std::optional<Event> get(std::chrono::milliseconds);

        ::rdma_event_channel* raw() noexcept;

    private:
        EventChannel(::rdma_event_channel*, int);

        void close();

    private:
        ::rdma_event_channel* _raw;
        int _epollFd = {0};
    };
}
