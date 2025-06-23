#include <mxl/mxl.h>
#include <mxl/platform.h>
#include <rdma/fabric.h>
#include <spdlog/spdlog.h>

void mxlFabricsTestInternal()
{
    uint32_t version = fi_version();

    SPDLOG_INFO("libfabric version: {}.{}", FI_MAJOR(version), FI_MINOR(version));
}
