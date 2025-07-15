#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <rdma/fi_eq.h>
#include "FIInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class ConnNotificationEntry
    {
    public:
        enum class EventType : uint32_t
        {
            ConnReq,
            Connected,
            Shutdown
        };

        static EventType from(uint32_t eventType)
        {
            switch (eventType)
            {
                case FI_CONNREQ:   return EventType::ConnReq;
                case FI_CONNECTED: return EventType::Connected;
                case FI_SHUTDOWN:  return EventType::Shutdown;
                default:           throw std::invalid_argument("Invalid event type for ConnNotificationEntry");
            }
        }

        static std::shared_ptr<ConnNotificationEntry> from_raw(::fi_eq_cm_entry const* raw, uint32_t eventType);

        [[nodiscard]]
        std::optional<FIInfoView> getInfo();

        [[nodiscard]]
        std::optional<fid_t> getFid() const;

        [[nodiscard]]
        EventType getEventType() const;

    private:
        ConnNotificationEntry(std::optional<::fid_t>, std::optional<FIInfoList>, EventType);

        std::optional<::fid_t> _fid;
        std::optional<FIInfoList> _info;
        EventType _eventType;
    };
}