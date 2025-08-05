#pragma once

#include <cstdint>
#include <rdma/fi_eq.h>
#include "FIInfo.hpp"
#include "variant"

namespace mxl::lib::fabrics::ofi
{

    class Event
    {
    public:
        class ConnectionRequested final
        {
        public:
            ConnectionRequested(::fid_t fid, FIInfo info);

            [[nodiscard]]
            ::fid_t fid() const noexcept;

            [[nodiscard]]
            FIInfoView info() const noexcept;

        private:
            ::fid_t _fid;
            FIInfo _info;
        };

        class Connected final
        {
        public:
            Connected(::fid_t fid);

            [[nodiscard]]
            ::fid_t fid() const noexcept;

        private:
            ::fid_t _fid;
        };

        class Shutdown final
        {
        public:
            Shutdown(::fid_t fid);

            [[nodiscard]]
            ::fid_t fid() const noexcept;

        private:
            ::fid_t _fid;
        };

        using Inner = std::variant<ConnectionRequested, Connected, Shutdown>;

        static Event fromRaw(::fi_eq_cm_entry const* raw, uint32_t eventType);

        [[nodiscard]]
        bool isConnReq() const noexcept;

        [[nodiscard]]
        bool isConnected() const noexcept;

        [[nodiscard]]
        bool isShutdown() const noexcept;

        [[nodiscard]]
        FIInfoView info() const;

        [[nodiscard]]
        fid_t fid() noexcept;

    private:
        Event(Inner);
        Inner _event;
    };
}
