#include "ConnectionManagement.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <optional>
#include <utility>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <spdlog/common.h>
#include "internal/Logging.hpp"
#include "Address.hpp"
#include "CompletionQueue.hpp"
#include "EventChannel.hpp"
#include "Exception.hpp"
#include "ProtectionDomain.hpp"
#include "QueuePair.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    ConnectionManagement ConnectionManagement::create()
    {
        auto ec = EventChannel::create();

        ::rdma_cm_id* raw;
        rdmaCall(rdma_create_id, "Failed to create a CM id", ec->raw(), &raw, nullptr, RDMA_PS_TCP);
        return {raw, std::move(ec)};
    }

    ConnectionManagement::ConnectionManagement(ConnectionManagement&& other) noexcept
        : _raw(other._raw)
        , _ec(std::move(other._ec))
        , _pd(std::move(other._pd))
        , _cq(std::move(other._cq))
    {
        other._raw = nullptr;
    }

    ConnectionManagement& ConnectionManagement::operator=(ConnectionManagement&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        _ec = std::move(other._ec);
        _pd = std::move(other._pd);
        _cq = std::move(other._cq);

        return *this;
    }

    void ConnectionManagement::bind(Address& addr)
    {
        auto ai = addr.aiPassive();
        rdmaCall(rdma_bind_addr, "Failed to bind addr to CM id", _raw, ai.raw()->ai_src_addr);
    }

    void ConnectionManagement::listen()
    {
        rdmaCall(rdma_listen, "Failed to listen", _raw, 8);
    }

    ConnectionManagement ConnectionManagement::waitConnectionRequest(size_t retries)
    {
        for (size_t retry = 0; retry < retries; retry++)
        {
            if (auto event = _ec->get(); event.isSuccess() && event.isConnectionRequest())
            {
                auto clientId = event.clientId();
                MXL_INFO("listenId->pd={:p} clientId->pd{:p}", (void*)_raw->pd, (void*)clientId->pd);
                return {event.clientId(), std::move(_ec), std::move(_pd), std::move(_cq)};
            }
        }

        throw Exception::internal("Failed to receive \"Connection Request cm event\"");
    }

    void ConnectionManagement::accept()
    {
        rdmaCall(rdma_accept, "Failed to accept connection", _raw, nullptr);
        if (auto event = _ec->get(); event.isSuccess() && event.isConnectionEstablished())
        {
            MXL_INFO("Connected!");
        }
        else
        {
            throw Exception::internal("Failed to receive \"Connection Established cm event\"");
        }
    }

    ProtectionDomain& ConnectionManagement::pd()
    {
        if (_pd)
        {
            return *_pd;
        }
        throw Exception::internal("Failed to get protection domain, because it was not created yet");
    }

    void ConnectionManagement::resolveAddr(Address& dstAddr, std::chrono::milliseconds timeout)
    {
        ;
        if (rdma_resolve_addr(_raw, nullptr, dstAddr.aiActive().raw()->ai_dst_addr, timeout.count()))
        {
            throw Exception::internal("Failed to resolve address");
        }

        if (auto event = _ec->get(); event.isSuccess() && event.isAddrResolved())
        {
            MXL_INFO("Address Resolved");
            return;
        }

        throw Exception::internal("Failed to receive \"Address resolved cm event\"");
    }

    void ConnectionManagement::resolveRoute(std::chrono::milliseconds timeout)
    {
        if (rdma_resolve_route(_raw, timeout.count()))
        {
            throw Exception::internal("Failed to resolve route");
        }
        if (auto event = _ec->get(); event.isSuccess() && event.isRouteResolved())
        {
            MXL_INFO("Route resolved!");
            return;
        }

        throw Exception::internal("Failed to receive \"Route resolved cm event\"");
    }

    void ConnectionManagement::connect()
    {
        ::rdma_conn_param param = {};
        param.initiator_depth = 3;
        param.responder_resources = 3;
        param.retry_count = 3;
        if (rdma_connect(_raw, &param))
        {
            throw Exception::internal("Failed to connect to server");
        }

        if (auto event = _ec->get(); event.isSuccess() && event.isConnectionEstablished())
        {
            MXL_INFO("Connected!");
            return;
        }

        throw Exception::internal("Failed to receiver \"Connection established cm event\"");
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
            throw Exception::internal("Failed to create Queue Pair");
        }
    }

    void ConnectionManagement::write(std::uint64_t id, LocalRegion& localRegion, RemoteRegion& remoteRegion)
    {
        ::ibv_send_wr wr = {}, *badWr;

        auto sge = localRegion.toSge();

        wr.wr_id = id;
        wr.sg_list = &sge;
        wr.num_sge = 1;
        wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
        wr.send_flags = IBV_SEND_SIGNALED;
        wr.imm_data = 0;
        wr.wr.rdma.remote_addr = remoteRegion.addr;
        wr.wr.rdma.rkey = remoteRegion.rkey;
        rdmaCall(ibv_post_send, "Failed to post remote write operation", _raw->qp, &wr, &badWr);
    }

    void ConnectionManagement::recv(LocalRegion& localRegion)
    {
        ::ibv_recv_wr recvWr = {}, *badWr;

        auto sge = localRegion.toSge();

        recvWr.next = nullptr;
        recvWr.num_sge = 1;
        recvWr.sg_list = &sge;
        recvWr.wr_id = 0xDEADBEEFA110BABE;

        if (ibv_post_recv(_raw->qp, &recvWr, &badWr))
        {
            MXL_ERROR("Failed to post recv operation: {}", strerror(errno));

            throw Exception::internal("Failed to post recv operation");
        }
    }

    std::optional<Completion> ConnectionManagement::readCq()
    {
        if (_cq)
        {
            return _cq->read();
        }
        return std::nullopt;
    }

    std::optional<Completion> ConnectionManagement::readCqBlocking(std::chrono::steady_clock::duration timeout)
    {
        auto timeoutMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
        if (timeoutMs.count() == 0)
        {
            return readCq();
        }

        if (_cq)
        {
            return _cq->readBlocking(timeoutMs);
        }
        return std::nullopt;
    }

    ConnectionManagement::~ConnectionManagement()
    {
        close();
    }

    ConnectionManagement::ConnectionManagement(::rdma_cm_id* raw, std::shared_ptr<EventChannel> ec, std::optional<ProtectionDomain> pd,
        std::optional<CompletionQueue> cq)
        : _raw(raw)
        , _ec(std::move(ec))
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
    }

}
