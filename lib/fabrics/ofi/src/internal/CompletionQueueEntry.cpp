#include "CompletionQueueEntry.hpp"
#include <optional>
#include <variant>
#include <rdma/fi_eq.h>
#include "mxl/mxl.h"
#include "Exception.hpp"
#include "VariantUtils.hpp"

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

    std::string CompletionQueueErrorEntry::toString(::fid_cq* cq) const
    {
        return ::fi_cq_strerror(cq, _raw.prov_errno, _raw.err_data, nullptr, 0);
    }

    CompletionEntry::CompletionEntry(CompletionQueueDataEntry entry)
        : _inner(entry)
    {}

    CompletionEntry::CompletionEntry(CompletionQueueErrorEntry entry)
        : _inner(entry)
    {}

    CompletionQueueDataEntry CompletionEntry::data()
    {
        return std::visit(
            overloaded{
                [](CompletionQueueDataEntry const& data) -> CompletionQueueDataEntry
                {
                    // Nothing to do, we are waiting for a setup call
                    return data;
                },
                [](CompletionQueueErrorEntry const&) -> CompletionQueueDataEntry
                { throw Exception::make(MXL_ERR_UNKNOWN, "Failed to unwrap Completion queue entry as CompletionQueueDataEntry"); },
            },
            _inner);
    }

    CompletionQueueErrorEntry CompletionEntry::err()
    {
        return std::visit(overloaded{[](CompletionQueueDataEntry const&) -> CompletionQueueErrorEntry
                              { throw Exception::make(MXL_ERR_UNKNOWN, "Failed to unwrap Completion queue entry as CompletionQueueErrorEntry"); },
                              [](CompletionQueueErrorEntry const& error) -> CompletionQueueErrorEntry
                              {
                                  return error;
                              }},
            _inner);
    }

    std::optional<CompletionQueueDataEntry> CompletionEntry::tryData() const noexcept
    {
        return std::visit(
            overloaded{
                [](CompletionQueueDataEntry const& data) -> std::optional<CompletionQueueDataEntry> { return data; },
                [](CompletionQueueErrorEntry const&) -> std::optional<CompletionQueueDataEntry> { return std::nullopt; },

            },
            _inner);
    }

    std::optional<CompletionQueueErrorEntry> CompletionEntry::tryErr() const noexcept
    {
        return std::visit(
            overloaded{
                [](CompletionQueueDataEntry const&) -> std::optional<CompletionQueueErrorEntry> { return std::nullopt; },
                [](CompletionQueueErrorEntry const& error) -> std::optional<CompletionQueueErrorEntry> { return error; },

            },
            _inner);
    }

    bool CompletionEntry::isDataEntry() const noexcept
    {
        return std::visit(
            overloaded{
                [](CompletionQueueDataEntry const&) -> bool { return true; },
                [](CompletionQueueErrorEntry const&) -> bool { return false; },
            },
            _inner);
    }

    bool CompletionEntry::isErrEntry() const noexcept
    {
        return std::visit(
            overloaded{
                [](CompletionQueueDataEntry const&) -> bool { return false; },
                [](CompletionQueueErrorEntry const&) -> bool { return true; },
            },
            _inner);
    }
}
