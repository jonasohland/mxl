#pragma once

#include <chrono>
#include <optional>
#include "CompletionQueue.hpp"
#include "Endpoint.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    enum class QueueReadMode
    {
        Blocking,
        NonBlocking,
    };

    template<QueueReadMode qrm>
    std::optional<Completion> readCompletionQueue(ActiveEndpoint& aep, std::chrono::steady_clock::duration)
    {
        std::optional<Completion> completion;

        if constexpr (qrm == QueueReadMode::Blocking)
        {
            completion = aep.readCqBlocking(); // TODO: find a way to enforce duration
        }
        else if constexpr (qrm == QueueReadMode::NonBlocking)
        {
            completion = aep.readCq();
        }
        else
        {
            static_assert(false, "Unsupported queue behaviour parameter");
        }

        return completion;
    }

}
