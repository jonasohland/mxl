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
#include "Region.hpp"

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

        static std::shared_ptr<Endpoint> create(std::shared_ptr<Domain> domain, std::optional<FIInfoView> remoteInfo = std::nullopt);

        void bind(std::shared_ptr<EventQueue> eq);
        void bind(std::shared_ptr<CompletionQueue> cq, uint64_t flags);

        void enable();

        void accept();
        void connect(FabricAddress const& addr);
        void shutdown();

        FabricAddress localAddress()
        {
            return FabricAddress::fromFid(&_raw->fid);
        }

        [[nodiscard]]
        std::shared_ptr<CompletionQueue> completionQueue() const;

        [[nodiscard]]
        std::shared_ptr<EventQueue> eventQueue() const;

        ::fid_ep* raw() noexcept;

        [[nodiscard]]
        ::fid_ep const* raw() const noexcept;

        /**
         * Push a remote write work request to the endpoint.
         * \param region The local memory region to write from.
         * \param remoteAddr The remote base address to write to
         * \param mrDesc The memory region descriptor. This is obtained by registering the memory region.
         * \param rkey The remote protection key provided by the target
         * \param destAddr The destination address of the target endpoint. This is unused when using connected endpoints.
         */
        void write(Region const& region, uint64_t remoteAddr, void* mrDesc, uint64_t rkey, ::fi_addr_t destAddr = FI_ADDR_UNSPEC);

        /**
         * Push a remote write work request to the endpoint. This version allows to specify multiple non contiguous regions.
         * \param region The local memory regions to write from.
         * \param remoteAddr The remote base address to write to
         * \param mrDesc The memory region descriptor. This is obtained by registering the memory region.
         * \param rkey The remote protection key provided by the target
         * \param destAddr The destination address of the target endpoint. This is unused when using connected endpoints.
         */
        void write(Regions const& regions, uint64_t remoteAddr, void** mrDesc, uint64_t rkey, ::fi_addr_t destAddr = FI_ADDR_UNSPEC);

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
