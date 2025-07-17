#include "CompletionQueueEntry.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::optional<uint64_t> CompletionQueueDataEntry::data() const noexcept
    {
        if (!(_raw.flags & FI_REMOTE_CQ_DATA))
        {
            return std::nullopt;
        }

        return _raw.data;
    }

    bool CompletionQueueDataEntry::isRemoteWrite() const noexcept
    {
        return (_raw.flags & FI_RMA) && (_raw.flags & FI_REMOTE_WRITE);
    }

    bool CompletionQueueDataEntry::isLocalWrite() const noexcept
    {
        return (_raw.flags & FI_RMA) && (_raw.flags & FI_WRITE);
    }
}