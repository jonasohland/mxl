#include "Endpoint.hpp"
#include <cerrno>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <spdlog/common.h>
#include "internal/Logging.hpp"
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

    ActiveEndpoint PassiveEndpoint::connect(Address& dstAddr)
    {
        _cm.connect(dstAddr);
        return std::move(*this);
    }

    void ActiveEndpoint::write(std::uint64_t id, LocalRegion& localRegion, RemoteRegion& remoteRegion)
    {
        ::ibv_send_wr wr, *badWr;

        auto sge = localRegion.toSge();

        wr.wr_id = id;
        wr.next = nullptr;
        wr.sg_list = &sge;
        wr.num_sge = 1;
        wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
        wr.send_flags = IBV_SEND_SOLICITED; // TODO: check if this is correct..
        wr.imm_data = id;
        wr.wr.rdma.remote_addr = remoteRegion.addr;
        wr.wr.rdma.rkey = remoteRegion.rkey;

        if (ibv_post_send(_cm.raw()->qp, &wr, &badWr))
        {
            throw std::runtime_error(fmt::format("Failed to post remote write operation: {}", strerror(errno)));
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

    ConnectionManagement& ActiveEndpoint::connectionManagement() noexcept
    {
        return _cm;
    }

    PassiveEndpoint PassiveEndpoint::create(Address& bindAddr, QueuePairAttr qpAttr)
    {
        auto cm = ConnectionManagement::create();

        cm.bind(bindAddr);

        MXL_INFO("addr binded");

        cm.createProtectionDomain();

        MXL_INFO("Created protection domain");

        cm.createCompletionQueue();

        MXL_INFO("Created Completion Queue");

        cm.createQueuePair(std::move(qpAttr));

        MXL_INFO("Created QueuePair");

        return {std::move(cm)};
    }

    void PassiveEndpoint::listen()
    {
        _cm.listen();
    }

    ActiveEndpoint::ActiveEndpoint(PassiveEndpoint&& pep)
        : _cm{std::move(pep._cm)}
    {}

    ActiveEndpoint PassiveEndpoint::waitConnectionRequest()
    {
        return {_cm.waitConnectionRequest()};
    }

    ConnectionManagement& PassiveEndpoint::connectionManagement() noexcept
    {
        return _cm;
    }

    PassiveEndpoint::PassiveEndpoint(ConnectionManagement cm)
        : _cm(std::move(cm))
    {}

}
