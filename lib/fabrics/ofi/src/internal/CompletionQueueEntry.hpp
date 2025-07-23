#pragma once

#include <optional>
#include <rdma/fi_eq.h>

namespace mxl::lib::fabrics::ofi
{
    class CompletionQueueDataEntry final
    {
    public:
        CompletionQueueDataEntry(::fi_cq_data_entry& raw)
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
}
