#include "QueuePair.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include "ProtectionDomain.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    QueuePairAttr QueuePairAttr::defaults() noexcept
    {
        return {.qpType = IBV_QPT_RC};
    }

    ::ibv_qp_init_attr QueuePairAttr::toIbv() const noexcept
    {
        ::ibv_qp_init_attr attr = {};
        attr.qp_type = qpType;
        attr.sq_sig_all = 1;
        attr.cap.max_send_wr = 8;
        attr.cap.max_send_sge = 1;
        attr.cap.max_recv_sge = 1;
        attr.cap.max_recv_wr = 8;
        return attr;
    }

    QueuePair::QueuePair(ProtectionDomain& pd, QueuePairAttr attr)
    {
        auto attrRaw = attr.toIbv();
        _raw = ibv_create_qp(pd.raw(), &attrRaw);
        if (!_raw)
        {
            throw std::runtime_error(fmt::format("Failed to create queue pair: {}", strerror(errno)));
        }
    }

    QueuePair::~QueuePair()
    {
        close();
    }

    QueuePair::QueuePair(QueuePair&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    QueuePair& QueuePair::operator=(QueuePair&& other)
    {
        close();

        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    ::ibv_qp* QueuePair::raw() noexcept
    {
        return _raw;
    }

    void QueuePair::close()
    {
        if (_raw)
        {
            if (ibv_destroy_qp(_raw))
            {
                throw std::runtime_error(fmt::format("Failed to destroy queue pair: {}", strerror(errno)));
            }
        }
    }

}
