#include "CompletionChannel.hpp"
#include <optional>
#include <infiniband/verbs.h>
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
        return {raw};
    }

    CompletionChannel::~CompletionChannel()
    {
        close();
    }

    CompletionChannel::CompletionChannel(CompletionChannel&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    CompletionChannel& CompletionChannel::operator=(CompletionChannel&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    std::optional<Event> CompletionChannel::get(ibv_cq* cq)
    {
        void* ctx = nullptr;

        if (auto ret = ::ibv_get_cq_event(_raw, &cq, &ctx); ret == 0)
        {
            rdmaCall(ibv_req_notify_cq, "Failed to request cq notify", cq, 0);
            return {Event{cq}};
        }

        return std::nullopt;
    }

    ::ibv_comp_channel* CompletionChannel::raw() noexcept
    {
        return _raw;
    }

    CompletionChannel::CompletionChannel(::ibv_comp_channel* raw)
        : _raw(raw)
    {}

    void CompletionChannel::close()
    {
        if (_raw)
        {
            rdmaCall(ibv_destroy_comp_channel, "Failed to destroy completion channel", _raw);
        }
    }

}
