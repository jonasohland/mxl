#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <infiniband/verbs.h>
#include "CompletionChannel.hpp"

namespace mxl::lib::fabrics::rdma_core
{

    class ConnectionManagement;

    class Completion
    {
    public:
        Completion(::ibv_wc raw)
            : _raw(raw)
        {}

        [[nodiscard]]
        bool isErr() const noexcept;

        [[nodiscard]]
        std::uint32_t immData() const noexcept;

        [[nodiscard]]
        ::ibv_wc_opcode opCode() const noexcept;

        [[nodiscard]]
        std::uint64_t wrId() const noexcept;

        [[nodiscard]]
        std::string errToString() const;

    private:
        ::ibv_wc _raw;
    };

    class CompletionQueue
    {
    public:
        ~CompletionQueue(); // Copy constructor is deleted

        CompletionQueue(CompletionQueue const&) = delete;
        void operator=(CompletionQueue const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        CompletionQueue(CompletionQueue&&) noexcept;
        // Move-assigning an endpoint to another releases all resources from the moved-into endpoint and
        CompletionQueue& operator=(CompletionQueue&&);

        std::optional<Completion> read();
        std::optional<Completion> readBlocking();

        ::ibv_cq* raw() noexcept;

    private:
        void close();

    private:
        friend class ConnectionManagement;

    private:
        CompletionQueue(ConnectionManagement& cm);
        ::ibv_cq* _raw;

        CompletionChannel _cc;
    };
}
