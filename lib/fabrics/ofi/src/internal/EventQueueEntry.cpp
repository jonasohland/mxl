#include "EventQueueEntry.hpp"
#include <memory>
#include <optional>

namespace mxl::lib::fabrics::ofi
{
    std::shared_ptr<ConnNotificationEntry> ConnNotificationEntry::from_raw(::fi_eq_cm_entry const* entry, uint32_t eventType)
    {
        struct MakeSharedEnabler : public ConnNotificationEntry
        {
            MakeSharedEnabler(std::optional<::fid_t> fid, std::optional<FIInfoList> info, EventType eventType)
                : ConnNotificationEntry(std::move(fid), std::move(info), eventType)
            {}
        };

        switch (eventType)
        {
            case FI_CONNREQ: {
                return std::make_shared<MakeSharedEnabler>(
                    std::make_optional(entry->fid), std::make_optional(FIInfoList::owned(entry->info)), from(eventType));
            }
            break;

            case FI_CONNECTED:
            case FI_SHUTDOWN:  {
                return std::make_shared<MakeSharedEnabler>(std::make_optional(entry->fid), std::nullopt, from(eventType));
            }
            break;

            default: throw std::invalid_argument("Invalid event type for ConnNotificationEntry");
        }
    }

    std::optional<FIInfoView> ConnNotificationEntry::getInfo() noexcept
    {
        if (!_info)
        {
            return std::nullopt;
        }

        return std::make_optional(*_info->begin());
    }

    std::optional<fid_t> ConnNotificationEntry::getFid() const noexcept
    {
        return _fid;
    }

    ConnNotificationEntry::EventType ConnNotificationEntry::getEventType() const noexcept
    {
        return _eventType;
    }

    ConnNotificationEntry::ConnNotificationEntry(std::optional<fid_t> fid, std::optional<FIInfoList> info, EventType eventType)
        : _fid(std::move(fid))
        , _info(std::move(info))
        , _eventType(EventType(eventType))
    {}
}