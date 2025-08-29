#include "CompletionQueue.hpp"
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>
#include "internal/Logging.hpp"
#include "CompletionChannel.hpp"
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
        : _cc(CompletionChannel::create(cm))
    {
        _raw = ibv_create_cq(cm.raw()->verbs, 128, nullptr, _cc.raw(), 0);
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
        , _cc(std::move(other._cc))
    {
        other._raw = nullptr;
    }

    CompletionQueue& CompletionQueue::operator=(CompletionQueue&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _cc = std::move(other._cc);

        return *this;
    }

    ::ibv_cq* CompletionQueue::raw() noexcept
    {
        return _raw;
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

    std::optional<Completion> CompletionQueue::readBlocking(std::chrono::milliseconds timeout)
    {
        if (auto event = _cc.get(_raw, timeout); event)
        {
            return read();
        }

        return std::nullopt;
    }

    void CompletionQueue::close()
    {
        if (_raw)
        {
            rdmaCall(ibv_destroy_cq, "Failed to destroy completion queue", _raw);
            _raw = nullptr;
        }
    }
}
