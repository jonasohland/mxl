#pragma once

#include <chrono>
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_eq.h>
#include "EventQueueEntry.hpp"
#include "Fabric.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct EventQueueAttr final

    {
        size_t size;

        static EventQueueAttr defaults();

        [[nodiscard]]
        ::fi_eq_attr into_raw() const noexcept;
    };

    class EventQueue
    {
    public:
        ~EventQueue();

        EventQueue(EventQueue const&) = delete;
        void operator=(EventQueue const&) = delete;

        EventQueue(EventQueue&&) noexcept;
        EventQueue& operator=(EventQueue&&);

        ::fid_eq* raw() noexcept;
        [[nodiscard]]
        ::fid_eq const* raw() const noexcept;

        static std::shared_ptr<EventQueue> open(std::shared_ptr<Fabric>, EventQueueAttr const& attr = EventQueueAttr::defaults());

        std::optional<Event> readEntry();
        std::optional<Event> readEntryBlocking(std::chrono::steady_clock::duration timeout);

    private:
        void close();

        EventQueue(::fid_eq* raw, std::shared_ptr<Fabric> fabric);

        std::optional<Event> handleReadResult(ssize_t, uint32_t, ::fi_eq_cm_entry*);

        ::fid_eq* _raw;
        std::shared_ptr<Fabric> _fabric;
    };
}
