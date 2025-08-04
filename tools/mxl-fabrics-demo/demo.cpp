#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <uuid.h>
#include <CLI/CLI.hpp>
#include <mxl/fabrics.h>
#include <mxl/flow.h>
#include <mxl/mxl.h>
#include <mxl/time.h>
#include <sys/types.h>
#include "../../lib/fabrics/ofi/src/internal/Base64.hpp"
#include "../../lib/src/internal/FlowParser.hpp"
#include "../../lib/src/internal/Logging.hpp"

/*
    Example how to use:

        1- Start a target: ./mlx-fabrics-sample -f bda7a5e7-32c1-483a-ae1e-055568cc4335 --node 2.2.2.2 --service 1234 --provider verbs -c flow.json
        2- Paste the target info that gets printed in stdout to the -t argument of the sender.
        3- Start a sender: ./mlx-fabrics-sample -s -f bda7a5e7-32c1-483a-ae1e-055568cc4335 --node 1.1.1.1 --service 1234 --provider verbs -t
   <targetInfo>
*/

std::sig_atomic_t volatile g_exit_requested = 0;

struct Config
{
    // flow configuration
    std::string const& flowID;

    // endpoint configuration
    std::string const& node;
    std::string const& service;
    mxlFabricsProvider provider;
};

void signal_handler(int)

{
    g_exit_requested = 1;
}

static mxlStatus runInitiator(mxlInstance instance, mxlFabricsInstance fabricsInstance, Config const& config, mxlTargetInfo_t* targetInfo);
static mxlStatus runTarget(mxlInstance instance, mxlFabricsInstance fabricsInstance, Config const& config, std::string const& flowConfigFile);

int main(int argc, char** argv)
{
    std::signal(SIGINT, &signal_handler);
    std::signal(SIGTERM, &signal_handler);

    CLI::App app("mxl-fabrics-sample");

    std::string flowConfigFile;
    auto _ = app.add_option(
        "-c, --flow-config-file", flowConfigFile, "The json file which contains the NMOS Flow configuration. Only used when configured as a target.");

    std::string domain;
    auto domainOpt = app.add_option("-d,--domain", domain, "The MXL domain directory");
    domainOpt->required(true);
    domainOpt->check(CLI::ExistingDirectory);

    std::string flowID;
    app.add_option("-f, --flow-id", flowID, "The flow ID. When configured as an initiator, this is the flow ID to read from.");

    bool runAsInitiator;
    auto runAsInitiatorOpt = app.add_flag("-i,--initiator",
        runAsInitiator,
        "Run as an initiator (flow reader + fabrics initiator). If not set, run as a receiver (fabrics target + flow writer).");
    runAsInitiatorOpt->default_val(true);

    std::string node;
    auto nodeOpt = app.add_option("-n,--node",
        node,
        "This corresponds to the interface identifier of the fabrics endpoint, it can also be a logical address. This can be seen as the bind "
        "address when using sockets.");
    nodeOpt->default_val("localhost");

    std::string service;
    auto serviceOpt = app.add_option("--service",
        service,
        "This corresponds to a service identifier for the fabrics endpoint. This can be seen as the bind port when using sockets.");
    serviceOpt->default_val("1234");

    std::string provider;
    auto providerOpt = app.add_option("-p,--provider", provider, "The fabrics provider. One of (tcp, verbs or efa). Default is 'tcp'.");
    providerOpt->default_val("tcp");

    std::string targetInfo;
    app.add_option("--target-info",
        targetInfo,
        "The target information. This is used when configured as an initiator . This is the target information to send to."
        "You first start the target and it will print the targetInfo that you paste to this argument");

    CLI11_PARSE(app, argc, argv);

    mxlFabricsProvider mxlProvider;
    auto status = mxlFabricsProviderFromString(provider.c_str(), &mxlProvider);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to parse provider '{}'", provider);
        return EXIT_FAILURE;
    }

    auto config = Config{
        .flowID = flowID,
        .node = node,
        .service = service,
        .provider = mxlProvider,
    };

    int exit_status = EXIT_SUCCESS;

    auto instance = mxlCreateInstance(domain.c_str(), "");
    if (instance == nullptr)
    {
        MXL_ERROR("Failed to create MXL instance");
        exit_status = EXIT_FAILURE;
        goto mxl_cleanup;
    }

    mxlFabricsInstance fabricsInstance;
    status = mxlFabricsCreateInstance(instance, &fabricsInstance);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create fabrics instance with status '{}'", static_cast<int>(status));
        return EXIT_FAILURE;
    }

    if (runAsInitiator)
    {
        mxlTargetInfo mxlTargetInfo;

        auto decodedStr = base64::from_base64(targetInfo);
        status = mxlFabricsTargetInfoFromString(decodedStr.c_str(), &mxlTargetInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to parse target info '{}'", targetInfo);
            exit_status = EXIT_FAILURE;
            goto mxl_cleanup;
        }

        exit_status = runInitiator(instance, fabricsInstance, config, mxlTargetInfo);

        mxlFabricsFreeTargetInfo(mxlTargetInfo);
    }
    else
    {
        exit_status = runTarget(instance, fabricsInstance, config, flowConfigFile);
    }

mxl_cleanup:
    if (instance != nullptr)
    {
        mxlDestroyInstance(instance);
    }

    return exit_status;
}

static mxlStatus runInitiator(mxlInstance instance, mxlFabricsInstance fabricsInstance, Config const& config, mxlTargetInfo_t* targetInfo)
{
    mxlFlowReader reader;

    // Create a flow reader for the given flow id.
    mxlStatus status = mxlCreateFlowReader(instance, config.flowID.c_str(), "", &reader);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create flow reader with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlFabricsInitiator initiator;
    status = mxlFabricsCreateInitiator(fabricsInstance, &initiator);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create fabrics initiator with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlFlowData flowData;
    status = mxlFlowReaderGetFlowData(reader, &flowData);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get flow data with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlRegions regions;
    status = mxlFabricsRegionsFromFlow(flowData, &regions);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get flow memory region with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlInitiatorConfig initiatorConfig = {
        .endpointAddress = {.node = config.node.c_str(), .service = config.service.c_str()},
        .provider = config.provider,
        .regions = regions,
        .deviceSupport = true,
    };

    status = mxlFabricsInitiatorSetup(initiator, &initiatorConfig);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to setup fabrics initiator with status '{}'", static_cast<int>(status));
        return status;
    }

    status = mxlFabricsInitiatorAddTarget(initiator, targetInfo);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to add target with status '{}'", static_cast<int>(status));
        return status;
    }

    // Extract the FlowInfo structure.
    FlowInfo flow_info;
    status = mxlFlowReaderGetInfo(reader, &flow_info);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get flow info with status '{}'", static_cast<int>(status));
        return status;
    }

    GrainInfo grainInfo;
    uint8_t* payload;

    // uint64_t grainIndex = flow_info.discrete.headIndex + 1;
    uint64_t grainIndex = mxlGetCurrentHeadIndex(&flow_info.discrete.grainRate);

    while (!g_exit_requested)
    {
        auto ret = mxlFlowReaderGetGrain(reader, grainIndex, 200000000, &grainInfo, &payload);
        if (ret == MXL_ERR_OUT_OF_RANGE_TOO_LATE)
        {
            // We are too late.. time travel!
            grainIndex = mxlGetCurrentHeadIndex(&flow_info.discrete.grainRate);
            continue;
        }
        if (ret == MXL_ERR_OUT_OF_RANGE_TOO_EARLY)
        {
            // We are too early somehow.. retry the same grain later.
            continue;
        }
        if (ret == MXL_ERR_TIMEOUT)
        {
            // No grains available before a timeout was triggered.. most likely a problem upstream.
            continue;
        }
        if (ret != MXL_STATUS_OK)
        {
            // Something  unexpected occured, not much we can do, but log and retry
            MXL_ERROR("Missed grain {}, err : {}", grainIndex, (int)ret);

            continue;
        }

        // Okay the grain is ready, we can transfer it to the targets.
        ret = mxlFabricsInitiatorTransferGrain(initiator, grainIndex);
        if (ret != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to transfer grain with status '{}'", static_cast<int>(ret));
            return status;
        }

        if (grainInfo.commitedSize != grainInfo.grainSize)
        {
            // partial commit, we will need to work on the same grain again.
            continue;
        }

        // If we get here, we have transfered the grain completely, we can work on the next grain.
        grainIndex++;
    }

    return MXL_STATUS_OK;
}

static mxlStatus runTarget(mxlInstance instance, mxlFabricsInstance fabricsInstance, Config const& config, std::string const& flowConfigFile)
{
    std::ifstream file(flowConfigFile, std::ios::in | std::ios::binary);
    if (!file)
    {
        MXL_ERROR("Failed to open file: '{}'", flowConfigFile);
        return MXL_ERR_INVALID_ARG;
    }
    std::string flowDescriptor{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    mxl::lib::FlowParser descriptorParser{flowDescriptor};

    auto flowId = uuids::to_string(descriptorParser.getId());

    FlowInfo flowInfo;
    auto status = mxlCreateFlow(instance, flowDescriptor.c_str(), nullptr, &flowInfo);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create flow with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlFlowWriter writer;
    // Create a flow writer for the given flow id.
    status = mxlCreateFlowWriter(instance, flowId.c_str(), "", &writer);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create flow writer with status '{}'", static_cast<int>(status));
    }

    mxlFlowData flowData;
    status = mxlFlowWriterGetFlowData(writer, &flowData);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get flow data with status '{}'", static_cast<int>(status));
        return status;
    }

    mxlRegions regions;
    status = mxlFabricsRegionsFromFlow(flowData, &regions);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get flow memory region with status '{}'", static_cast<int>(status));
        return status;
    }

    // create and setup the fabrics target
    mxlFabricsTarget target;
    mxlTargetInfo targetInfo;

    mxlTargetConfig targetConfig = {
        .endpointAddress = {.node = config.node.c_str(), .service = config.service.c_str()},
        .provider = config.provider,
        .regions = regions,
        .deviceSupport = true,
    };

    status = mxlFabricsCreateTarget(fabricsInstance, &target);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to create fabrics target with status '{}'", static_cast<int>(status));
        return status;
    }

    status = mxlFabricsTargetSetup(fabricsInstance, target, &targetConfig, &targetInfo);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to setup fabrics target with status '{}'", static_cast<int>(status));
        return status;
    }

    auto targetInfoStr = std::string{};
    size_t targetInfoStrSize;

    status = mxlFabricsTargetInfoToString(targetInfo, nullptr, &targetInfoStrSize);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to get target info string size with status '{}'", static_cast<int>(status));
        return status;
    }
    targetInfoStr.resize(targetInfoStrSize);

    status = mxlFabricsTargetInfoToString(targetInfo, targetInfoStr.data(), &targetInfoStrSize);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to convert target info to string with status '{}'", static_cast<int>(status));
        return status;
    }

    MXL_INFO("Target info:  {}", base64::to_base64(targetInfoStr));

    GrainInfo dummyGrainInfo;
    uint64_t grainIndex = 0;
    uint8_t* dummyPayload;
    while (!g_exit_requested)
    {
        status = mxlFabricsTargetWaitForNewGrain(target, &grainIndex, 200);
        if (status == MXL_ERR_TIMEOUT)
        {
            // No completion before a timeout was triggered, most likely a problem upstream.
            // MXL_WARN("wait for new grain timeout, most likely there is a problem upstream.");
            continue;
        }

        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to wait for grain with status '{}'", static_cast<int>(status));
            return status;
        }

        // Here we open so that we can commit, we are not going to modify the grain as it was already modified by the initiator.
        status = mxlFlowWriterOpenGrain(writer, grainIndex, &dummyGrainInfo, &dummyPayload);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to open grain with status '{}'", static_cast<int>(status));
            return status;
        }

        // GrainInfo and media payload was already written by the remote endpoint, we simply commit!.
        status = mxlFlowWriterCommitGrain(writer, &dummyGrainInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to commit grain with status '{}'", static_cast<int>(status));
            return status;
        }

        MXL_INFO("Comitted grain with index={} commitedSize={} grainSize={}", grainIndex, dummyGrainInfo.commitedSize, dummyGrainInfo.grainSize);
    }

    return MXL_STATUS_OK;
}
