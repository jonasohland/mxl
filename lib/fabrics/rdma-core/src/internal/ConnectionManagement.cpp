#include "ConnectionManagement.hpp"
#include <cerrno>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <arpa/inet.h>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>
#include <spdlog/common.h>
#include "internal/Logging.hpp"
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "ProtectionDomain.hpp"
#include "QueuePair.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    ConnectionManagement ConnectionManagement::create()
    {
        auto ec = ::rdma_create_event_channel();
        if (!ec)
        {
            throw std::runtime_error(fmt::format("Failed to create event channel: {}", strerror(errno)));
        }

        ::rdma_cm_id* raw;
        if (rdma_create_id(ec, &raw, nullptr, RDMA_PS_TCP))
        {
            rdma_destroy_event_channel(ec);

            throw std::runtime_error(fmt::format("Failed to create a CM id: {}", strerror(errno)));
        }

        return {raw, ec};
    }

    ConnectionManagement::ConnectionManagement(ConnectionManagement&& other) noexcept
        : _raw(other._raw)
        , _ec(other._ec)
        , _srcAddr(std::move(other._srcAddr))
        , _pd(std::move(other._pd))
        , _cq(std::move(other._cq))
    {
        other._raw = nullptr;
        other._ec = nullptr;
    }

    ConnectionManagement& ConnectionManagement::operator=(ConnectionManagement&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;
        other._ec = nullptr;

        other._srcAddr = std::move(_srcAddr);
        other._pd = std::move(_pd);
        other._cq = std::move(_cq);

        return *this;
    }

    void ConnectionManagement::bind(Address& addr)
    {
        auto ai = addr.aiPassive();
        if (rdma_bind_addr(_raw, ai.raw()->ai_src_addr))
        {
            throw std::runtime_error(fmt::format("Failed to bind addr to CM id: {}", strerror(errno)));
        }
        _srcAddr = std::move(ai);
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
        ::rdma_cm_event* event;

        if (rdma_get_cm_event(_ec, &event))
        {
            throw std::runtime_error(fmt::format("Failed to get a CM event: {}", strerror(errno)));
        }

        auto aep = event->id;
        aep->qp = _raw->qp;
        aep->send_cq = _cq->_raw;
        aep->recv_cq = _cq->_raw;
        if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
        {
            if (rdma_accept(aep, &event->param.conn))
            {
                rdma_ack_cm_event(event);
                throw std::runtime_error(fmt::format("Failed to accept connection request: {}", strerror(errno)));
            }
        }
        else
        {
            throw std::runtime_error(fmt::format("Expected a connection request event, but got a different event: {}", rdma_event_str(event->event)));
        }

        if (rdma_ack_cm_event(event))
        {
            throw std::runtime_error(fmt::format("Failed to ACK event: {}", strerror(errno)));
        }

        // rdma_cm_id* aep;
        // if (rdma_get_request(_raw, &aep))
        // {
        //     throw std::runtime_error(fmt::format("Failed to get rdma connection request: {}", strerror(errno)));
        // }
        //
        return {aep, nullptr, std::move(_srcAddr), std::move(_pd), std::move(_cq)};
    }

    void ConnectionManagement::connect(Address& dstAddr)
    {
        if (!_srcAddr)
        {
            return;
        }

        auto* localAddr = ::rdma_get_local_addr(_raw);

        // This resolves the destination address
        if (rdma_resolve_addr(_raw, localAddr, dstAddr.aiActive().raw()->ai_dst_addr, 2000))
        {
            MXL_INFO("Resolve addr fail?");
            throw std::runtime_error(fmt::format("Failed to resolve destination address: {}", strerror(errno))); // TODO: adjust this timeout
        }
        MXL_INFO("Address Resolved!");

        // Wait for RDMA_CM_EVENT_ADDR_RESOLVED

        // This resolves the route
        if (rdma_resolve_route(_raw, 2000))
        {
            throw std::runtime_error(fmt::format("Failed to resolve route: {}", strerror(errno))); // TODO: adjust this timeout
        }
        MXL_INFO("Route Resolved!");
        // Wait for RDMA_CM_EVENT_ROUTE_RESOLVED

        if (rdma_connect(_raw, nullptr))
        {
            throw std::runtime_error(fmt::format("Failed to connect: {}", strerror(errno)));
        }
        MXL_INFO("rdma_connect successful!");
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
        if (!_cq)
        {
            createCompletionQueue();
        }

        auto attrRaw = attr.toIbv();
        attrRaw.send_cq = _cq->_raw;
        attrRaw.recv_cq = _cq->_raw;
        if (rdma_create_qp(_raw, _pd->raw(), &attrRaw))
        {
            throw std::runtime_error(fmt::format("Failed to create Queue Pair: ", strerror(errno)));
        }
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

    ConnectionManagement::~ConnectionManagement()
    {
        close();
    }

    ConnectionManagement::ConnectionManagement(::rdma_cm_id* raw, ::rdma_event_channel* ec, std::optional<AddressInfo> srcAddr,
        std::optional<ProtectionDomain> pd, std::optional<CompletionQueue> cq)
        : _raw(raw)
        , _ec(ec)
        , _srcAddr(std::move(srcAddr))
        , _pd(std::move(pd))
        , _cq(std::move(cq))
    {}

    ::rdma_cm_id* ConnectionManagement::raw() noexcept
    {
        return _raw;
    }

    void ConnectionManagement::close()
    {
        if (_raw)
        {
            rdma_destroy_id(_raw);
            _raw = nullptr;
        }

        if (_ec)
        {
            rdma_destroy_event_channel(_ec);
            _ec = nullptr;
        }
    }

}
