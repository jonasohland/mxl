#pragma once

#include <chrono>
#include <optional>
#include "CompletionQueue.hpp"
#include "ConnectionManagement.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    enum class QueueReadMode
    {
        Blocking,
        NonBlocking,
    };

    template<QueueReadMode qrm>
    std::optional<Completion> readCompletionQueue(ConnectionManagement& cm, std::chrono::steady_clock::duration timeout)
    {
        std::optional<Completion> completion;

        if constexpr (qrm == QueueReadMode::Blocking)
        {
            completion = cm.readCqBlocking(timeout);
        }
        else if constexpr (qrm == QueueReadMode::NonBlocking)
        {
            completion = cm.readCq();
        }
        else
        {
            static_assert(false, "Unsupported queue behaviour parameter");
        }

        return completion;
    }

}
