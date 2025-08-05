#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <rdma/fi_eq.h>

namespace mxl::lib::fabrics::ofi
{
    class CompletionQueue;

    class CompletionQueueDataEntry final
    {
    public:
        explicit CompletionQueueDataEntry(::fi_cq_data_entry& raw)
            : _raw(raw)
        {}

        [[nodiscard]]
        std::optional<uint64_t> data() const noexcept;
        [[nodiscard]]
        bool isRemoteWrite() const noexcept;
        [[nodiscard]]
        bool isLocalWrite() const noexcept;

    private:
        ::fi_cq_data_entry _raw;
    };

    class CompletionQueueErrorEntry final
    {
    public:
        explicit CompletionQueueErrorEntry(::fi_cq_err_entry& raw, std::shared_ptr<CompletionQueue> queue)
            : _raw(raw)
            , _queue(std::move(queue))
        {}

        [[nodiscard]]
        std::string toString() const;

    private:
        ::fi_cq_err_entry _raw;
        std::shared_ptr<CompletionQueue> _queue;
    };

    class CompletionEntry
    {
    public:
        explicit CompletionEntry(CompletionQueueDataEntry entry);
        explicit CompletionEntry(CompletionQueueErrorEntry entry);

        CompletionQueueDataEntry data();
        CompletionQueueErrorEntry err();

        [[nodiscard]]
        std::optional<CompletionQueueDataEntry> tryData() const noexcept;
        [[nodiscard]]
        std::optional<CompletionQueueErrorEntry> tryErr() const noexcept;

        [[nodiscard]]
        bool isDataEntry() const noexcept;
        [[nodiscard]]
        bool isErrEntry() const noexcept;

    private:
        using Inner = std::variant<CompletionQueueDataEntry, CompletionQueueErrorEntry>;

        Inner _inner;
    };
}
