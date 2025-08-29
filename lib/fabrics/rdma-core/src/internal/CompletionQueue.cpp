#include "CompletionQueue.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>
#include "internal/Logging.hpp"
#include "ConnectionManagement.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    bool Completion::isErr() const noexcept
    {
        return _raw.status != IBV_WC_SUCCESS;
    }

    std::uint32_t Completion::immData() const noexcept
    {
        return _raw.imm_data;
    }

    ::ibv_wc_opcode Completion::opCode() const noexcept
    {
        return _raw.opcode;
    }

    std::uint64_t Completion::wrId() const noexcept
    {
        return _raw.wr_id;
    }

    std::string Completion::errToString() const
    {
        if (!isErr())
        {
            throw std::runtime_error("Completion is not an error, can't convert to error string.");
        }

        return ibv_wc_status_str(_raw.status);
    }

    CompletionQueue::CompletionQueue(ConnectionManagement& cm)
    {
        _cc = ibv_create_comp_channel(cm.raw()->verbs);
        if (!_cc)
        {
            throw Exception::internal("Failed to create completion channel");
        }

        _raw = ibv_create_cq(cm.raw()->verbs, 128, nullptr, _cc, 0);
        if (!_raw)
        {
            throw Exception::internal("Failed to create completion queue");
        }

        rdmaCall(ibv_req_notify_cq, "Failed to register completion queue notify", _raw, 0);
    }

    CompletionQueue::~CompletionQueue()
    {
        close();
    }

    CompletionQueue::CompletionQueue(CompletionQueue&& other) noexcept
        : _raw(other._raw)
        , _cc(other._cc)
    {
        other._raw = nullptr;
        other._cc = nullptr;
    }

    CompletionQueue& CompletionQueue::operator=(CompletionQueue&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _cc = other._cc;
        other._cc = nullptr;

        return *this;
    }

    std::optional<Completion> CompletionQueue::read()
    {
        ::ibv_wc wc;

        auto ret = ibv_poll_cq(_raw, 1, &wc);
        if (ret < 0)
        {
            MXL_ERROR("Failed to poll completion queue: {}", strerror(errno));
            throw std::runtime_error(fmt::format("Failed to poll completion queue: {}", strerror(errno)));
        }
        if (ret == 1)
        {
            return Completion{std::move(wc)};
        }
        return std::nullopt;
    }

    std::optional<Completion> CompletionQueue::readBlocking()
    {
        if (auto completion = read(); completion)
        {
            return completion;
        }

        // TODO: wrap cq events to its own class
        void* ctx = nullptr;
        rdmaCall(ibv_get_cq_event, "Failed to get cq event", _cc, &_raw, &ctx);

        ibv_req_notify_cq(_raw, 0);
        auto completion = read();

        ibv_ack_cq_events(_raw, 1);

        return completion;
    }

    void CompletionQueue::close()
    {
        if (_raw)
        {
            rdmaCall(ibv_destroy_cq, "Failed to destroy completion queue", _raw);
            _raw = nullptr;
        }

        if (_cc)
        {
            rdmaCall(ibv_destroy_comp_channel, "Failed to destroy completion channel", _cc);
        }
    }
}
