#include "CompletionQueue.hpp"
#include <cerrno>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>
#include "ConnectionManagement.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    bool Completion::isErr() const noexcept
    {
        return _raw.status != IBV_WC_SUCCESS;
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
        _raw = ibv_create_cq(cm.raw()->verbs, 128, nullptr, nullptr, 0); // TODO: review the zeroes
        if (!_raw)
        {
            throw std::runtime_error(fmt::format("Failed to create completion queue: {}", strerror(errno)));
        }
    }

    CompletionQueue::~CompletionQueue()
    {
        close();
    }

    CompletionQueue::CompletionQueue(CompletionQueue&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    CompletionQueue& CompletionQueue::operator=(CompletionQueue&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    std::optional<Completion> CompletionQueue::read()
    {
        ::ibv_wc wc;

        auto ret = ibv_poll_cq(_raw, 1, &wc);
        if (ret < 0)
        {
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
        throw std::runtime_error("Not implemented yet!");
    }

    void CompletionQueue::close()
    {
        if (_raw)
        {
            if (ibv_destroy_cq(_raw))
            {
                throw std::runtime_error(fmt::format("Failed to destroy completion queue: {}", strerror(errno)));
            }
        }
    }
}
