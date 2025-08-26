#include "ConnectionManagement.hpp"
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "ProtectionDomain.hpp"
#include "QueuePair.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    ConnectionManagement ConnectionManagement::create()
    {
        ::rdma_cm_id* raw;
        if (rdma_create_id(nullptr, &raw, nullptr, RDMA_PS_TCP))
        {
            throw std::runtime_error(fmt::format("Failed to create a CM id: {}", strerror(errno)));
        }

        return {raw};
    }

    ConnectionManagement::ConnectionManagement(ConnectionManagement&& other) noexcept
        : _raw(other._raw)
        , _srcAddr(std::move(other._srcAddr))
        , _pd(std::move(other._pd))
        , _cq(std::move(other._cq))
        , _qp(std::move(other._qp))
    {
        other._raw = nullptr;
    }

    ConnectionManagement& ConnectionManagement::operator=(ConnectionManagement&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        other._srcAddr = std::move(_srcAddr);
        other._pd = std::move(_pd);
        other._cq = std::move(_cq);
        other._qp = std::move(_qp);

        return *this;
    }

    void ConnectionManagement::bind(Address& addr)
    {
        auto ai = addr.aiPassive();
        if (rdma_bind_addr(_raw, ai.raw()->ai_src_addr))
        {
            throw std::runtime_error(fmt::format("Failed to bind addr to CM id: {}", strerror(errno)));
        }
    }

    void ConnectionManagement::listen()
    {
        if (rdma_listen(_raw, 0))
        {
            throw std::runtime_error(fmt::format("Failed to listen: {}", strerror(errno)));
        }
    }

    ConnectionManagement ConnectionManagement::waitConnectionRequest()
    {
        ::rdma_cm_id* peer;
        if (rdma_get_request(_raw, &peer))
        {
            throw std::runtime_error(fmt::format("Faield to get connection request: {}", strerror(errno)));
        }

        return {peer};
    }

    void ConnectionManagement::connect(Address& dstAddr)
    {
        if (!_srcAddr)
        {
            return;
        }

        // This resolves the destination address
        if (rdma_resolve_addr(_raw, _srcAddr->raw()->ai_src_addr, dstAddr.aiActive().raw()->ai_dst_addr, 2000))
        {
            throw std::runtime_error(fmt::format("Failed to resolve destination address: {}", strerror(errno))); // TODO: adjust this timeout
        }

        // This resolves the route
        if (rdma_resolve_route(_raw, 2000))
        {
            throw std::runtime_error(fmt::format("Failed to resolve route: {}", strerror(errno))); // TODO: adjust this timeout
        }

        if (rdma_connect(_raw, nullptr))
        {
            throw std::runtime_error(fmt::format("Failed to connect: {}", strerror(errno)));
        }
    }

    void ConnectionManagement::createProtectionDomain() noexcept
    {
        _pd = ProtectionDomain{*this};
    }

    void ConnectionManagement::createCompletionQueue() noexcept
    {
        _cq = CompletionQueue{*this};
    }

    void ConnectionManagement::createQueuePair(QueuePairAttr attr)
    {
        if (!_pd)
        {
            createProtectionDomain();
        }

        _qp = QueuePair{*_pd, std::move(attr)};
    }

    ProtectionDomain& ConnectionManagement::protectionDomain()
    {
        if (!_pd)
        {
            throw std::runtime_error(fmt::format("Failed to get protection domain, because it was not created yet!"));
        }
        return *_pd;
    }

    CompletionQueue& ConnectionManagement::completionQueue()
    {
        if (!_cq)
        {
            throw std::runtime_error(fmt::format("Failed to get completion queue, because it was not created yet!"));
        }
        return *_cq;
    }

    QueuePair& ConnectionManagement::queuePair()
    {
        if (!_qp)
        {
            throw std::runtime_error(fmt::format("Failed to get queue pair, because it was not created yet!"));
        }
        return *_qp;
    }

    ConnectionManagement::~ConnectionManagement()
    {
        close();
    }

    ConnectionManagement::ConnectionManagement(::rdma_cm_id* raw)
        : _raw(raw)
    {}

    void ConnectionManagement::close()
    {
        if (_raw)
        {
            rdma_destroy_id(_raw);
        }
    }

}
