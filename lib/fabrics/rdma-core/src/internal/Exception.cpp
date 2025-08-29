// SPDX - FileCopyrightText : 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "Exception.hpp"
#include "mxl/mxl.h"

namespace mxl::lib::fabrics::rdma_core
{
    Exception::Exception(std::string msg, mxlStatus status)
        : _msg(std::move(msg))
        , _status(status)
    {}

    mxlStatus Exception::status() const noexcept
    {
        return _status;
    }

    char const* Exception::what() const noexcept
    {
        return _msg.c_str();
    }

    RdmaException::RdmaException(std::string msg, mxlStatus status, int rdmaErrno)
        : Exception(std::move(msg), status)
        , _errno(rdmaErrno)
    {}

    int RdmaException::rdmaErrno() const noexcept
    {
        return _errno;
    }

    mxlStatus mxlStatusFromRdmaErrno(int rdmaErrno)
    {
        switch (rdmaErrno)
        {
            // case -FI_EINTR:  return MXL_ERR_INTERRUPTED;
            // case -FI_EAGAIN: return MXL_ERR_NOT_READY;
            default: return MXL_ERR_UNKNOWN;
        }
    }
}
