// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cerrno>
#include <cstring>
#include <exception>
#include <string>
#include <fmt/format.h>
#include <mxl/mxl.h>

namespace mxl::lib::fabrics::rdma_core
{

    mxlStatus mxlStatusFromRdmaErrno(int rdmaErrno);

    class Exception : public std::exception
    {
    public:
        Exception(std::string msg, mxlStatus status);

        template<typename... T>
        static Exception make(mxlStatus status, fmt::format_string<T...> fmt, T&&... args)
        {
            return Exception(fmt::format(fmt, std::forward<T>(args)...), status);
        }

        template<typename... T>
        static Exception invalidArgument(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_INVALID_ARG, fmt, std::forward<T>(args)...);
        }

        template<typename... T>
        static Exception internal(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_INTERNAL, fmt, std::forward<T>(args)...);
        }

        template<typename... T>
        static Exception invalidState(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_INVALID_STATE, fmt, std::forward<T>(args)...);
        }

        template<typename... T>
        static Exception exists(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_EXISTS, fmt, std::forward<T>(args)...);
        }

        template<typename... T>
        static Exception notFound(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_NOT_FOUND, fmt, std::forward<T>(args)...);
        }

        template<typename... T>
        static Exception interrupted(fmt::format_string<T...> fmt, T&&... args)
        {
            return make(MXL_ERR_INTERRUPTED, fmt, std::forward<T>(args)...);
        }

        [[nodiscard]]
        mxlStatus status() const noexcept;

        [[nodiscard]]
        char const* what() const noexcept override;

    private:
        std::string _msg;
        mxlStatus _status;
    };

    class RdmaException : public Exception
    {
    public:
        RdmaException(std::string msg, mxlStatus status, int rdmaErrno);

        template<typename... T>
        static RdmaException make(int rdmaErrno, fmt::format_string<T...> fmt, T&&... args)
        {
            return RdmaException(fmt::format(fmt, std::forward<T>(args)...), mxlStatusFromRdmaErrno(rdmaErrno), rdmaErrno);
        }

        [[nodiscard]]
        int rdmaErrno() const noexcept;

    private:
        int _errno;
    };

    template<typename F, typename... T>
    void rdmaCall(F fun, std::string_view msg, T... args)
    {
        auto result = fun(std::forward<T>(args)...);
        if (result != 0)
        {
            auto str = ::strerror(result);
            throw RdmaException::make(result, "{}: {}, code {}", msg, str, result);
        }
    }
}
