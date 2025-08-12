#pragma once

namespace mxl::lib::fabrics::ofi
{
    class RDMTarget
    {
    public:
        // Destructor releases all resources.
        ~RDMTarget();

        // Copy constructor is deleted
        RDMTarget(RDMTarget const&) = delete;
        void operator=(RDMTarget const&) = delete;

        // Implements proper move semantics. An endpoint in a moved-from state can no longer be used.
        RDMTarget(RDMTarget&&) noexcept;
        RDMTarget& operator=(RDMTarget&&);

    private:
    };
}
