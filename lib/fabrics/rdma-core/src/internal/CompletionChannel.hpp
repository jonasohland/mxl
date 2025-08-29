#pragma once

#include <chrono>
#include <optional>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

namespace mxl::lib::fabrics::rdma_core
{
    class CompletionQueue;
    class ConnectionManagement;

    class CompletionChannel
    {
    public:
        class Event
        {
        public:
            Event(ibv_cq* cq);
            ~Event();
            Event(Event const&) = delete;
            void operator=(Event const&) = delete;
            Event(Event&&) = default;
            Event& operator=(Event&&) = delete;

        private:
            ibv_cq* _cq;
        };

    public:
        static CompletionChannel create(ConnectionManagement& cm);

        ~CompletionChannel();
        CompletionChannel(CompletionChannel const&) = delete;
        void operator=(CompletionChannel const&) = delete;
        CompletionChannel(CompletionChannel&&) noexcept;
        CompletionChannel& operator=(CompletionChannel&&);

        std::optional<Event> get(ibv_cq* cq, std::chrono::milliseconds timeout);

        ::ibv_comp_channel* raw() noexcept;

    private:
        CompletionChannel(::ibv_comp_channel*, int);
        void close();

    private:
        ::ibv_comp_channel* _raw;
        int _epollFd = {0};
    };
}
