#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include "CompletionQueue.hpp"
#include "Domain.hpp"
#include "EventQueue.hpp"
#include "FIInfo.hpp"

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
        void connect(void const* addr);
        void shutdown();

        [[nodiscard]]
        std::shared_ptr<CompletionQueue> completionQueue() const;

        [[nodiscard]]
        std::shared_ptr<EventQueue> eventQueue() const;

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
