#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <rdma/fi_eq.h>
#include "FIInfo.hpp"
#include "variant"

namespace mxl::lib::fabrics::ofi
{

    class ConnNotificationEntry
    {
    public:
        ~ConnNotificationEntry() = default;
        ConnNotificationEntry(ConnNotificationEntry const&) = delete;
        void operator=(ConnNotificationEntry const&) = delete;

        ConnNotificationEntry(ConnNotificationEntry&&) noexcept = default;
        ConnNotificationEntry& operator=(ConnNotificationEntry&&) = default;

        class EventConnReq
        {
        public:
            EventConnReq(fid_t fid, FIInfoList info)
                : _fid(fid)
                , _info(std::move(info))
            {}

            [[nodiscard]]
            fid_t fid() noexcept
            {
                return _fid;
            }

            [[nodiscard]]
            FIInfoView info() noexcept
            {
                return *_info.begin();
            }

        private:
            fid_t _fid;
            FIInfoList _info;
        };

        class EventConnected
        {
        public:
            EventConnected(fid_t fid)
                : _fid(fid)
            {}

            [[nodiscard]]
            fid_t fid() noexcept
            {
                return _fid;
            }

        private:
            fid_t _fid;
        };

        class EventShutdown
        {
        public:
            EventShutdown(fid_t fid)
                : _fid(fid)
            {}

            [[nodiscard]]
            fid_t fid() noexcept
            {
                return _fid;
            }

        private:
            fid_t _fid;
        };

        using Event = std::variant<EventConnReq, EventConnected, EventShutdown>;

        static std::shared_ptr<ConnNotificationEntry> from_raw(::fi_eq_cm_entry const* raw, uint32_t eventType);

        [[nodiscard]]
        bool isConnReq() const noexcept;

        [[nodiscard]]
        bool isConnected() const noexcept;

        [[nodiscard]]
        bool isShutdown() const noexcept;

        [[nodiscard]]
        std::optional<FIInfoView> info() noexcept;

        [[nodiscard]]
        std::optional<fid_t> fid() noexcept;

    private:
        ConnNotificationEntry(Event);
        Event _event;
    };
}