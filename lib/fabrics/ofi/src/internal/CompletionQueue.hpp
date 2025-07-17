#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <rdma/fi_eq.h>
#include "CompletionQueueEntry.hpp"
#include "Domain.hpp"

namespace mxl::lib::fabrics::ofi
{
    struct CompletionQueueAttr
    {
        size_t size;
        enum fi_wait_obj wait_obj;

        static CompletionQueueAttr get_default();

        [[nodiscard]]
        ::fi_cq_attr into_raw() const noexcept;
    };

    class CompletionQueue

    {
    public:
        ~CompletionQueue();

        CompletionQueue(CompletionQueue const&) = delete;
        void operator=(CompletionQueue const&) = delete;

        CompletionQueue(CompletionQueue&&) noexcept;
        CompletionQueue& operator=(CompletionQueue&&);

        ::fid_cq* raw() noexcept;
        [[nodiscard]]
        ::fid_cq const* raw() const noexcept;

        static std::shared_ptr<CompletionQueue> open(std::shared_ptr<Domain> domain,
            CompletionQueueAttr const& attr = CompletionQueueAttr::get_default());

        std::optional<CompletionQueueDataEntry> tryEntry();
        std::optional<CompletionQueueDataEntry> waitForEntry(std::chrono::steady_clock::duration timeout);

    private:
        void close();

        CompletionQueue(::fid_cq* raw, std::shared_ptr<Domain> domain);

        std::optional<CompletionQueueDataEntry> handleReadResult(ssize_t ret, ::fi_cq_data_entry& entry);

        ::fid_cq* _raw;
        std::shared_ptr<Domain> _domain;
    };
}
