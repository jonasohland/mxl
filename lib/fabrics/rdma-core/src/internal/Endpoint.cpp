#include "Endpoint.hpp"
#include <cerrno>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include "Address.hpp"
#include "ConnectionManagement.hpp"
#include "LocalRegion.hpp"
#include "QueuePair.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    ActiveEndpoint::ActiveEndpoint(ConnectionManagement cm)
        : _cm(std::move(cm))
    {}

    ActiveEndpoint::ActiveEndpoint(PassiveEndpoint&& pep)
        : _cm{std::move(pep._cm)}
    {}

    ActiveEndpoint PassiveEndpoint::waitConnectionRequest()
    {
        return {_cm.waitConnectionRequest()};
    }

    ActiveEndpoint PassiveEndpoint::connect(Address& dstAddr)
    {
        _cm.connect(dstAddr);
        return std::move(*this);
    }

    void ActiveEndpoint::write(LocalRegion& localRegion, RemoteRegion& remoteRegion)
    {
        if (rdma_post_write(_cm.raw(),
                nullptr,
                reinterpret_cast<void*>(localRegion.addr),
                localRegion.len,
                nullptr, // TODO: ibv_mr should be carried in localRegion
                IBV_SEND_SOLICITED,
                remoteRegion.addr,
                remoteRegion.rkey))
        {
            throw std::runtime_error(fmt::format("Failed to post remote write operation: {}", strerror(errno)));
        }
    }

    void ActiveEndpoint::send(LocalRegion& localRegion)
    {
        if (rdma_post_send(_cm.raw(), nullptr, reinterpret_cast<void*>(localRegion.addr), localRegion.len, nullptr, 0)) // TODO: ibv_mr should be
                                                                                                                        // carried in localRegion
        {
            throw std::runtime_error(fmt::format("Failed to post send operation: {}", strerror(errno)));
        }
    }

    void ActiveEndpoint::recv(LocalRegion& localRegion)
    {
        if (rdma_post_recv(_cm.raw(), nullptr, reinterpret_cast<void*>(localRegion.addr), localRegion.len, nullptr))
        {
            throw std::runtime_error(fmt::format("Failed to post recv operation: {}", strerror(errno)));
        }
    }

    std::optional<Completion> ActiveEndpoint::readCq()
    {
        return _cm.completionQueue().read();
    }

    std::optional<Completion> ActiveEndpoint::readCqBlocking()
    {
        ::ibv_wc wc;
        auto ret = rdma_get_send_comp(_cm.raw(), &wc);
        if (ret == 1)
        {
            return Completion{std::move(wc)};
        }
        if (ret < 0)
        {
            throw std::runtime_error(fmt::format("Failed to read completion queue: {}", strerror(errno)));
        }

        return std::nullopt;
    }

    PassiveEndpoint PassiveEndpoint::create(Address& bindAddr, QueuePairAttr qpAttr)
    {
        auto cm = ConnectionManagement::create();

        cm.bind(bindAddr);
        cm.createProtectionDomain();
        cm.createCompletionQueue();
        cm.createQueuePair(std::move(qpAttr));

        return {std::move(cm)};
    }

    void PassiveEndpoint::listen()
    {
        _cm.listen();
    }

}
