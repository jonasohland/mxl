//
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace mxl::lib::fabrics::rdma_core
{
    // Allows us to use the visitor pattern with lambdas
    template<class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
}
