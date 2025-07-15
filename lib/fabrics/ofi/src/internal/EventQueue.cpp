#include "EventQueue.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include "internal/Logging.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"

namespace mxl::lib::fabrics::ofi
{

    EventQueueAttr EventQueueAttr::get_default()
    {
        EventQueueAttr attr{};
        attr.size = 8; // default size, this should be parameterized
        return attr;
    }

    ::fi_eq_attr EventQueueAttr::into_raw() const noexcept
    {
        ::fi_eq_attr raw{};
        raw.size = size;
        raw.wait_obj = FI_WAIT_UNSPEC; // default wait object, this should be parameterized
        raw.wait_set = nullptr;        // only used if wait_obj is FI_WAIT_SET
        raw.flags = 0;                 // if signaling vector is required, this should use the flag "FI_AFFINITY"
        raw.signaling_vector = 0;      // this should indicate the CPU core that interrupts associated with the eq should target.
        return raw;
    }

    std::shared_ptr<EventQueue> EventQueue::open(std::shared_ptr<Fabric> fabric, EventQueueAttr const& attr)
    {
        ::fid_eq* eq;
        auto eq_attr = attr.into_raw();

        fiCall(::fi_eq_open, "Failed to open event queue", fabric->raw(), &eq_attr, &eq, nullptr);

        // expose the private constructor to std::make_shared inside this function
        struct MakeSharedEnabler : public EventQueue
        {
            MakeSharedEnabler(::fid_eq* raw)
                : EventQueue(raw)
            {}
        };

        return std::make_shared<MakeSharedEnabler>(eq);
    }

    std::optional<std::shared_ptr<ConnNotificationEntry>> EventQueue::tryEntry()
    {
        uint32_t eventType;
        ::fi_eq_cm_entry entry;

        auto ret = fi_eq_read(_raw, &eventType, &entry, sizeof(entry), FI_PEEK);

        return handleReadResult(ret, eventType, &entry);
    }

    std::optional<std::shared_ptr<ConnNotificationEntry>> EventQueue::waitForEntry(std::chrono::steady_clock::duration timeout)
    {
        uint32_t eventType;
        ::fi_eq_cm_entry entry;

        auto ret = fi_eq_sread(_raw, &eventType, &entry, sizeof(entry), std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(), 0);

        return handleReadResult(ret, eventType, &entry);
    }

    EventQueue::EventQueue(::fid_eq* raw)
        : _raw(raw)
    {}

    EventQueue::~EventQueue()
    {
        close();
    }

    EventQueue::EventQueue(EventQueue&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    EventQueue& EventQueue::operator=(EventQueue&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    ::fid_eq* EventQueue::raw() noexcept
    {
        return _raw;
    }

    ::fid_eq const* EventQueue::raw() const noexcept
    {
        return _raw;
    }

    void EventQueue::close()
    {
        if (_raw)
        {
            fiCall(::fi_close, "Failed to close event queue", &_raw->fid);
            _raw = nullptr;
        }
    }

    std::optional<std::shared_ptr<ConnNotificationEntry>> EventQueue::handleReadResult(ssize_t ret, uint32_t eventType, ::fi_eq_cm_entry* entry)
    {
        if (ret == -FI_EAGAIN)
        {
            // No event available
            return std::nullopt;
        }

        if (ret < 0)
        {
            ::fi_eq_err_entry eq_err{};
            fi_eq_readerr(_raw, &eq_err, 0);

            MXL_ERROR("Failed to read an entry from the event queue with an error: \"{}\"",
                fi_eq_strerror(_raw, eq_err.prov_errno, eq_err.err_data, nullptr, 0));

            return std::nullopt;
        }

        // try to read the entry as a connection notification event, if it fails it is ok, because we have not consumed the event yet
        try
        {
            auto connEntry = ConnNotificationEntry::from_raw(entry, eventType);

            // If we got here, we know it's a connection notification event, we can safely consume the event from the queue
            fi_eq_read(_raw, &eventType, entry, sizeof(entry), 0);

            return connEntry;
        }
        catch (...)
        {
            // There is an entry in the event queue, but it is not a connection notification event
            return std::nullopt;
        }
    }
}