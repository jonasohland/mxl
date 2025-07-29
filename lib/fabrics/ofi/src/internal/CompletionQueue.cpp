#include "CompletionQueue.hpp"
#include <memory>
#include <optional>
#include <utility>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <sys/types.h>
#include "internal/Logging.hpp"
#include "CompletionQueueEntry.hpp"
#include "Domain.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    CompletionQueueAttr CompletionQueueAttr::get_default()
    {
        CompletionQueueAttr attr{};
        attr.size = 8;                  // default size, this should be parameterized
        attr.wait_obj = FI_WAIT_UNSPEC; // TODO: EFA will require no wait object
        return attr;
    }

    ::fi_cq_attr CompletionQueueAttr::into_raw() const noexcept
    {
        ::fi_cq_attr raw{};
        raw.size = size;
        raw.wait_obj = wait_obj;
        raw.format = FI_CQ_FORMAT_DATA;  // default format, this should be parameterized
        raw.wait_cond = FI_CQ_COND_NONE; // default condition, this should be parameterized
        raw.wait_set = nullptr;          // only used if wait_obj is FI_WAIT_SET
        raw.flags = 0;                   // if signaling vector is required, this should use the flag "FI_AFFINITY"
        raw.signaling_vector = 0;        // this should indicate the CPU core that interrupts associated with the cq should target.
        return raw;
    }

    std::shared_ptr<CompletionQueue> CompletionQueue::open(std::shared_ptr<Domain> domain, CompletionQueueAttr const& attr)
    {
        ::fid_cq* cq;
        auto cq_attr = attr.into_raw();

        fiCall(::fi_cq_open, "Failed to open completion queue", domain->raw(), &cq_attr, &cq, nullptr);

        // expose the private constructor to std::make_shared inside this function
        struct MakeSharedEnabler : public CompletionQueue
        {
            MakeSharedEnabler(::fid_cq* raw, std::shared_ptr<Domain> domain)
                : CompletionQueue(raw, domain)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(cq, domain);
    }

    std::optional<CompletionEntry> CompletionQueue::tryEntry()
    {
        fi_cq_data_entry entry;

        ssize_t ret = fi_cq_read(_raw, &entry, 1);

        return handleReadResult(ret, entry);
    }

    std::optional<CompletionEntry> CompletionQueue::waitForEntry(std::chrono::steady_clock::duration timeout)
    {
        fi_cq_data_entry entry;

        ssize_t ret = fi_cq_sread(_raw, &entry, 1, nullptr, std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        return handleReadResult(ret, entry);
    }

    CompletionQueue::CompletionQueue(::fid_cq* raw, std::shared_ptr<Domain> domain)
        : _raw(raw)
        , _domain(std::move(domain))
    {}

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

    ::fid_cq* CompletionQueue::raw() noexcept
    {
        return _raw;
    }

    ::fid_cq const* CompletionQueue::raw() const noexcept
    {
        return _raw;
    }

    void CompletionQueue::close()
    {
        if (_raw)
        {
            MXL_INFO("Closing completion queue");

            fiCall(::fi_close, "Failed to close completion queue", &_raw->fid);
            _raw = nullptr;
        }
    }

    std::optional<CompletionEntry> CompletionQueue::handleReadResult(ssize_t ret, ::fi_cq_data_entry& entry)
    {
        if (ret == -FI_EAGAIN)
        {
            // No entry available
            return std::nullopt;
        }

        if (ret == -FI_EAVAIL)
        {
            // An entry is available but in the error queue
            ::fi_cq_err_entry err_entry;
            fi_cq_readerr(_raw, &err_entry, 0);

            return CompletionEntry{CompletionQueueErrorEntry{err_entry}};
        }

        return CompletionEntry{CompletionQueueDataEntry{entry}};
    }
}
