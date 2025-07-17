#include "EventQueueEntry.hpp"
#include <memory>
#include <optional>
#include "FIInfo.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::shared_ptr<ConnNotificationEntry> ConnNotificationEntry::from_raw(::fi_eq_cm_entry const* entry, uint32_t eventType)
    {
        struct MakeSharedEnabler : public ConnNotificationEntry
        {
            MakeSharedEnabler(Event ev)
                : ConnNotificationEntry(std::move(ev))
            {}
        };

        switch (eventType)
        {
            case FI_CONNREQ:   return std::make_shared<MakeSharedEnabler>(EventConnReq{entry->fid, FIInfoList::owned(entry->info)});
            case FI_CONNECTED: return std::make_shared<MakeSharedEnabler>(EventConnected{entry->fid});
            case FI_SHUTDOWN:  return std::make_shared<MakeSharedEnabler>(EventShutdown{entry->fid});
            default:           throw std::invalid_argument("Invalid event type for ConnNotificationEntry");
        }
    }

    bool ConnNotificationEntry::isConnReq() const noexcept
    {
        return std::visit(overloaded{[](EventConnReq const&) { return true; },
                              [](EventConnected const&) { return false; },
                              [](EventShutdown const&)
                              {
                                  return false;
                              }},
            _event);
    }

    bool ConnNotificationEntry::isConnected() const noexcept
    {
        return std::visit(overloaded{[](EventConnReq const&) { return false; },
                              [](EventConnected const&) { return true; },
                              [](EventShutdown const&)
                              {
                                  return false;
                              }},
            _event);
    }

    bool ConnNotificationEntry::isShutdown() const noexcept
    {
        return std::visit(overloaded{[](EventConnReq const&) { return false; },
                              [](EventConnected const&) { return false; },
                              [](EventShutdown const&)
                              {
                                  return true;
                              }},
            _event);
    }

    std::optional<FIInfoView> ConnNotificationEntry::info() noexcept
    {
        return std::visit(overloaded{[](EventConnReq& ev) -> std::optional<FIInfoView> { return ev.info(); },
                              [](EventConnected&) -> std::optional<FIInfoView> { return std::nullopt; },
                              [](EventShutdown&) -> std::optional<FIInfoView>
                              {
                                  return std::nullopt;
                              }},
            _event);
    }

    std::optional<fid_t> ConnNotificationEntry::fid() noexcept
    {
        return std::visit(overloaded{[](EventConnReq& ev) { return ev.fid(); },
                              [](EventConnected& ev) { return ev.fid(); },
                              [](EventShutdown& ev)
                              {
                                  return ev.fid();
                              }},
            _event);
    }

    ConnNotificationEntry::ConnNotificationEntry(Event ev)
        : _event(std::move(ev))
    {}

}