#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"
#include "FIInfo.hpp"
#include "LocalRegion.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Endpoint
    {
    public:
        ~Endpoint();
        Endpoint(Endpoint const&) = delete;
        void operator=(Endpoint const&) = delete;

        Endpoint(Endpoint&&) noexcept;
        Endpoint& operator=(Endpoint&&);

        static Endpoint create(std::shared_ptr<Domain> domain, FIInfoView);

        void bind(std::shared_ptr<EventQueue> eq);
        void bind(std::shared_ptr<CompletionQueue> cq, uint64_t flags);

        void enable();

        void accept();
        void connect(FabricAddress const& addr);
        void shutdown();

        [[nodiscard]]
        FabricAddress localAddress() const;

        [[nodiscard]]
        std::shared_ptr<CompletionQueue> completionQueue() const;

        [[nodiscard]]
        std::shared_ptr<EventQueue> eventQueue() const;

        /**
         * Push a remote write work request to the endpoint work queue.
         * \param localGroup Source memory regions to write from
         * \param remoteGroup Destination memory regions to write to
         * \param destAddr The destination address of the target endpoint. This is unused when using connected endpoints.
         * \param 64 bits of user data that will be available in the completion entry associated with this transfer.
         */
        void write(LocalRegionGroup& localGroup, RemoteRegionGroup const& remoteGroup, ::fi_addr_t destAddr = FI_ADDR_UNSPEC,
            std::optional<uint64_t> immData = std::nullopt);

        /*
         * Push a recv work request to the endpoint work queue.
         * \param region Source memory region to receive data into

         */
        void recv(LocalRegion region);

        ::fid_ep* raw() noexcept;

        [[nodiscard]]
        ::fid_ep const* raw() const noexcept;

    private:
        void close();

        Endpoint(::fid_ep* raw, std::shared_ptr<Domain> domain, std::optional<std::shared_ptr<CompletionQueue>> cq = std::nullopt,
            std::optional<std::shared_ptr<EventQueue>> eq = std::nullopt);

        ::fid_ep* _raw;
        std::shared_ptr<Domain> _domain;

        std::optional<std::shared_ptr<CompletionQueue>> _cq;
        std::optional<std::shared_ptr<EventQueue>> _eq;
    };
}
