#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <uuid.h>
#include <CLI/CLI.hpp>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <mxl/fabrics.h>
#include <mxl/flow.h>
#include <mxl/mxl.h>
#include <mxl/time.h>
#include <sys/types.h>
#include "../../lib/fabrics/ofi/src/internal/Base64.hpp"
#include "../../lib/src/internal/FlowParser.hpp"
#include "../../lib/src/internal/Logging.hpp"
#include "CLI/CLI.hpp"

/*
    Example how to use:

        1- Start a target: ./mlx-fabrics-demo -f <flow-file-to-produce> --node 2.2.2.2 --service 1234 --provider verbs
        2- Paste the target info that gets printed in stdout to the --target-info argument of the initiator.
        3- Start a sender: ./mlx-fabrics-demo -i -f <flow-to-transfer> --node 1.1.1.1 --service 1234 --provider verbs --target-info <targetInfo>
   <targetInfo>
*/

std::sig_atomic_t volatile g_exit_requested = 0;

void signal_handler(int)
{
    g_exit_requested = 1;
}

struct Config
{
    std::string domain;

    // flow configuration
    mxl::lib::FlowParser descriptor;

    // endpoint configuration
    std::optional<std::string> node;
    std::optional<std::string> service;
    mxlFabricsProvider provider;

    bool requiresDeviceSupport;
    bool useCuda;
    int cudaDeviceId{0};
};

mxlStatus allocateCudaMemory(void** cudaBuf, mxlRegions* out_regions, int cudaDeviceId, size_t size)
{
    if (cudaMalloc(cudaBuf, size) != cudaSuccess)
    {
        MXL_ERROR("Failed to allocate CUDA memory");
    }

    mxlFabricsMemoryRegion region{
        reinterpret_cast<std::uintptr_t>(*cudaBuf),
        size,
        {MXL_MEMORY_REGION_TYPE_CUDA, static_cast<uint32_t>(cudaDeviceId)},
    };

    mxlFabricsMemoryRegionGroup group{&region, 1};

    return mxlFabricsRegionsFromBufferGroups(&group, 1, out_regions);
}

class AppInitator
{
public:
    AppInitator(Config config)
        : _config(std::move(config))
    {}

    ~AppInitator()
    {
        mxlStatus status;

        if (_cudaBuf != nullptr)
        {
            if (cudaFree(_cudaBuf) != cudaSuccess)
            {
                MXL_ERROR("Failed to free CUDA memory");
            }
            _cudaBuf = nullptr;
        }

        if (_targetInfo != nullptr)
        {
            if (status = mxlFabricsFreeTargetInfo(_targetInfo); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to free target info with status '{}'", static_cast<int>(status));
            }
        }

        if (_initiator != nullptr)
        {
            if (status = mxlFabricsDestroyInitiator(_fabricsInstance, _initiator); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy fabrics initiator with status '{}'", static_cast<int>(status));
            }
        }

        if (_fabricsInstance != nullptr)
        {
            if (status = mxlFabricsDestroyInstance(_fabricsInstance); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy fabrics instance with status '{}'", static_cast<int>(status));
            }
        }

        if (_reader != nullptr)
        {
            if (status = mxlReleaseFlowReader(_instance, _reader); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to release flow writer with status '{}'", static_cast<int>(status));
            }
        }

        if (_instance != nullptr)
        {
            if (status = mxlDestroyInstance(_instance); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy instance with status '{}'", static_cast<int>(status));
            }
        }
    }

    mxlStatus allocateCudaMemory(mxlRegions* out_regions)
    {
        return ::allocateCudaMemory(&_cudaBuf, out_regions, _config.cudaDeviceId, _config.descriptor.getPayloadSize());
    }

    mxlStatus setup(std::string targetInfoStr)
    {
        _instance = mxlCreateInstance(_config.domain.c_str(), "");
        if (_instance == nullptr)
        {
            MXL_ERROR("Failed to create MXL instance");
            return MXL_ERR_INVALID_ARG;
        }

        auto status = mxlFabricsCreateInstance(_instance, &_fabricsInstance);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create fabrics instance with status '{}'", static_cast<int>(status));
            return status;
        }

        // Create a flow reader for the given flow id.
        status = mxlCreateFlowReader(_instance, uuids::to_string(_config.descriptor.getId()).c_str(), "", &_reader);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create flow reader with status '{}'", static_cast<int>(status));
            return status;
        }

        status = mxlFabricsCreateInitiator(_fabricsInstance, &_initiator);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create fabrics initiator with status '{}'", static_cast<int>(status));
            return status;
        }

        // If we are using CUDA, we will allocate the memory regions on the CUDA device.        if (_config.useCuda)
        mxlRegions regions;
        if (_config.useCuda)
        {
            status = allocateCudaMemory(&regions);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to convert flow regions to CUDA regions with status '{}'", static_cast<int>(status));
                return status;
            }
        }

        else
        {
            status = mxlFabricsRegionsForFlowReader(_reader, &regions);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to get flow memory region with status '{}'", static_cast<int>(status));
                return status;
            }
        }
        status = mxlFabricsTargetInfoFromString(targetInfoStr.c_str(), &_targetInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to parse target info string with status '{}'", static_cast<int>(status));
            return status;
        }

        mxlInitiatorConfig initiatorConfig = {
            .endpointAddress = {.node = _config.node ? _config.node.value().c_str() : nullptr,
                                .service = _config.service ? _config.service.value().c_str() : nullptr},
            .provider = _config.provider,
            .regions = regions,
            .deviceSupport = _config.requiresDeviceSupport,
        };

        status = mxlFabricsInitiatorSetup(_initiator, &initiatorConfig);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to setup fabrics initiator with status '{}'", static_cast<int>(status));
            return status;
        }

        // After setting up the initiator, we can free the regions info object, the initator has kept a copy of that information during memory
        // registration.
        status = mxlFabricsRegionsFree(regions);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to free regions with status '{}'", static_cast<int>(status));
            return status;
        }

        status = mxlFabricsInitiatorAddTarget(_initiator, _targetInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to add target with status '{}'", static_cast<int>(status));
            return status;
        }

        do
        {
            status = mxlFabricsInitiatorMakeProgressBlocking(_initiator, 250);
            if (status == MXL_ERR_INTERRUPTED)
            {
                return MXL_STATUS_OK;
            }

            if (status != MXL_ERR_NOT_READY && status != MXL_STATUS_OK)
            {
                return status;
            }
        }
        while (status == MXL_ERR_NOT_READY);

        return MXL_STATUS_OK;
    }

    mxlStatus run()

    { // Extract the FlowInfo structure.
        FlowInfo flow_info;
        auto status = mxlFlowReaderGetInfo(_reader, &flow_info);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to get flow info with status '{}'", static_cast<int>(status));
            return status;
        }

        GrainInfo grainInfo;
        uint8_t* payload;

        // uint64_t grainIndex = flow_info.discrete.headIndex + 1;
        uint64_t grainIndex = mxlGetCurrentIndex(&flow_info.discrete.grainRate);

        while (!g_exit_requested)
        {
            auto ret = mxlFlowReaderGetGrain(_reader, grainIndex, 200000000, &grainInfo, &payload);
            if (ret == MXL_ERR_OUT_OF_RANGE_TOO_LATE)
            {
                // We are too late.. time travel!
                grainIndex = mxlGetCurrentIndex(&flow_info.discrete.grainRate);
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
            ret = mxlFabricsInitiatorTransferGrain(_initiator, grainIndex);
            if (ret == MXL_ERR_NOT_READY)
            {
                continue;
            }
            if (ret != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to transfer grain with status '{}'", static_cast<int>(ret));
                return status;
            }

            do
            {
                status = mxlFabricsInitiatorMakeProgressBlocking(_initiator, 10);
                if (status == MXL_ERR_INTERRUPTED)
                {
                    return MXL_STATUS_OK;
                }

                if (status != MXL_ERR_NOT_READY && status != MXL_STATUS_OK)
                {
                    return status;
                }
            }
            while (status == MXL_ERR_NOT_READY);

            if (grainInfo.commitedSize != grainInfo.grainSize)
            {
                // partial commit, we will need to work on the same grain again.
                continue;
            }

            // If we get here, we have transfered the grain completely, we can work on the next grain.
            grainIndex++;
        }

        status = mxlFabricsInitiatorRemoveTarget(_initiator, _targetInfo);
        if (status != MXL_STATUS_OK)
        {
            return status;
        }

        do
        {
            status = mxlFabricsInitiatorMakeProgressBlocking(_initiator, 250);
            if (status == MXL_ERR_INTERRUPTED)
            {
                return MXL_STATUS_OK;
            }
            if (status != MXL_ERR_NOT_READY && status != MXL_STATUS_OK)
            {
                return status;
            }
        }
        while (status == MXL_ERR_NOT_READY);

        return MXL_STATUS_OK;
    }

private:
    Config _config;

    mxlInstance _instance;
    mxlFabricsInstance _fabricsInstance;
    mxlFlowReader _reader;
    mxlFabricsInitiator _initiator;
    mxlTargetInfo _targetInfo;
    void* _cudaBuf{nullptr};
};

class AppTarget
{
public:
    AppTarget(Config config)
        : _config(std::move(config))
    {}

    ~AppTarget()
    {
        mxlStatus status;

        if (_cudaBuf != nullptr)
        {
            if (cudaFree(_cudaBuf) != cudaSuccess)
            {
                MXL_ERROR("Failed to free CUDA memory");
            }
            _cudaBuf = nullptr;
        }

        if (_targetInfo != nullptr)
        {
            if (status = mxlFabricsFreeTargetInfo(_targetInfo); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to free target info with status '{}'", static_cast<int>(status));
            }
        }

        if (_target != nullptr)
        {
            if (status = mxlFabricsDestroyTarget(_fabricsInstance, _target); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy target with status '{}'", static_cast<int>(status));
            }
        }

        if (_fabricsInstance != nullptr)
        {
            if (status = mxlFabricsDestroyInstance(_fabricsInstance); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy fabrics instance with status '{}'", static_cast<int>(status));
            }
        }

        if (_writer != nullptr)
        {
            if (status = mxlReleaseFlowWriter(_instance, _writer); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to release flow writer with status '{}'", static_cast<int>(status));
            }
        }

        if (_flowExits)
        {
            if (status = mxlDestroyFlow(_instance, uuids::to_string(_config.descriptor.getId()).c_str()); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy flow with status '{}'", static_cast<int>(status));
            }
        }

        if (_instance != nullptr)
        {
            if (status = mxlDestroyInstance(_instance); status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to destroy instance with status '{}'", static_cast<int>(status));
            }
        }
    }

    mxlStatus allocateCudaMemory(mxlRegions* out_regions)
    {
        return ::allocateCudaMemory(&_cudaBuf, out_regions, _config.cudaDeviceId, _config.descriptor.getPayloadSize());
    }

    mxlStatus setup(std::string const& flowDescriptor)
    {
        _instance = mxlCreateInstance(_config.domain.c_str(), "");
        if (_instance == nullptr)
        {
            MXL_ERROR("Failed to create MXL instance");
            return MXL_ERR_INVALID_ARG;
        }

        auto status = mxlFabricsCreateInstance(_instance, &_fabricsInstance);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create fabrics instance with status '{}'", static_cast<int>(status));
            return status;
        }

        FlowInfo flowInfo;
        status = mxlCreateFlow(_instance, flowDescriptor.c_str(), nullptr, &flowInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create flow with status '{}'", static_cast<int>(status));
            return status;
        }
        _flowExits = true;

        // Create a flow writer for the given flow id.
        status = mxlCreateFlowWriter(_instance, uuids::to_string(_config.descriptor.getId()).c_str(), "", &_writer);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create flow writer with status '{}'", static_cast<int>(status));
            return status;
        }

        mxlRegions memoryRegions;
        if (_config.useCuda)
        {
            status = allocateCudaMemory(&memoryRegions);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to convert flow regions to CUDA regions with status '{}'", static_cast<int>(status));
                return status;
            }
        }
        else
        {
            status = mxlFabricsRegionsForFlowWriter(_writer, &memoryRegions);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to get flow memory region with status '{}'", static_cast<int>(status));
                return status;
            }
        }

        status = mxlFabricsCreateTarget(_fabricsInstance, &_target);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to create fabrics target with status '{}'", static_cast<int>(status));
            return status;
        }

        mxlTargetConfig targetConfig = {
            .endpointAddress = {.node = _config.node ? _config.node.value().c_str() : nullptr,
                                .service = _config.service ? _config.service.value().c_str() : nullptr},
            .provider = _config.provider,
            .regions = memoryRegions,
            .deviceSupport = _config.requiresDeviceSupport,
        };
        status = mxlFabricsTargetSetup(_target, &targetConfig, &_targetInfo);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to setup fabrics target with status '{}'", static_cast<int>(status));
            return status;
        }

        status = mxlFabricsRegionsFree(memoryRegions);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to free memory regions with status '{}'", static_cast<int>(status));
            return status;
        }

        return MXL_STATUS_OK;
    }

    mxlStatus printInfo()
    {
        auto targetInfoStr = std::string{};
        size_t targetInfoStrSize;

        auto status = mxlFabricsTargetInfoToString(_targetInfo, nullptr, &targetInfoStrSize);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to get target info string size with status '{}'", static_cast<int>(status));
            return status;
        }
        targetInfoStr.resize(targetInfoStrSize);

        status = mxlFabricsTargetInfoToString(_targetInfo, targetInfoStr.data(), &targetInfoStrSize);
        if (status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to convert target info to string with status '{}'", static_cast<int>(status));
            return status;
        }

        MXL_INFO("Target info:  {}", base64::to_base64(targetInfoStr));

        return MXL_STATUS_OK;
    }

    mxlStatus run()
    {
        GrainInfo dummyGrainInfo;
        uint64_t grainIndex = 0;
        uint8_t* dummyPayload;
        mxlStatus status;

        while (!g_exit_requested)
        {
            status = mxlFabricsTargetWaitForNewGrain(_target, &grainIndex, 200);
            if (status == MXL_ERR_TIMEOUT)
            {
                // No completion before a timeout was triggered, most likely a problem upstream.
                // MXL_WARN("wait for new grain timeout, most likely there is a problem upstream.");
                continue;
            }

            if (status == MXL_ERR_INTERRUPTED)
            {
                return MXL_STATUS_OK;
            }

            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to wait for grain with status '{}'", static_cast<int>(status));
                return status;
            }

            // Here we open so that we can commit, we are not going to modify the grain as it was already modified by the initiator.
            status = mxlFlowWriterOpenGrain(_writer, grainIndex, &dummyGrainInfo, &dummyPayload);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to open grain with status '{}'", static_cast<int>(status));
                return status;
            }

            // GrainInfo and media payload was already written by the remote endpoint, we simply commit!.
            status = mxlFlowWriterCommitGrain(_writer, &dummyGrainInfo);
            if (status != MXL_STATUS_OK)
            {
                MXL_ERROR("Failed to commit grain with status '{}'", static_cast<int>(status));
                return status;
            }

            MXL_INFO("Comitted grain with index={} commitedSize={} grainSize={}", grainIndex, dummyGrainInfo.commitedSize, dummyGrainInfo.grainSize);
        }

        return MXL_STATUS_OK;
    }

private:
    Config _config;

    mxlInstance _instance;
    mxlFabricsInstance _fabricsInstance;
    mxlFlowWriter _writer;
    mxlFabricsTarget _target;
    mxlTargetInfo _targetInfo;
    void* _cudaBuf{nullptr};

    bool _flowExits{false};
};

int main(int argc, char** argv)
{
    std::signal(SIGINT, &signal_handler);
    std::signal(SIGTERM, &signal_handler);

    CLI::App app("mxl-fabrics-demo");

    std::string domain;
    auto domainOpt = app.add_option("-d,--domain", domain, "The MXL domain directory");
    domainOpt->required(true);
    domainOpt->check(CLI::ExistingDirectory);

    std::string flowConf;
    app.add_option("-f, --flow",
        flowConf,
        "The flow ID when used as an initiator. The json file which contains the NMOS Flow configuration when used as a target.");

    bool runAsInitiator;
    auto runAsInitiatorOpt = app.add_flag("-i,--initiator",
        runAsInitiator,
        "Run as an initiator (flow reader + fabrics initiator). If not set, run as a receiver (fabrics target + flow writer).");
    runAsInitiatorOpt->default_val(false);

    std::optional<std::string> node;
    auto nodeOpt = app.add_option("-n,--node",
        node,
        "This corresponds to the interface identifier of the fabrics endpoint, it can also be a logical address. This can be seen as the bind "
        "address when using sockets.");
    nodeOpt->default_val(std::nullopt);

    std::optional<std::string> service;
    auto serviceOpt = app.add_option("--service",
        service,
        "This corresponds to a service identifier for the fabrics endpoint. This can be seen as the bind port when using sockets.");
    serviceOpt->default_val(std::nullopt);

    std::string provider;
    auto providerOpt = app.add_option("-p,--provider", provider, "The fabrics provider. One of (tcp, verbs or efa). Default is 'tcp'.");
    providerOpt->default_val("tcp");

    std::string targetInfo;
    app.add_option("--target-info",
        targetInfo,
        "The target information. This is used when configured as an initiator . This is the target information to send to."
        "You first start the target and it will print the targetInfo that you paste to this argument");

    bool requiresDeviceSupport;
    auto requiresDeviceSupportOpt = app.add_flag("--device-support",
        requiresDeviceSupport,
        "This flag is used to specify that transfers will require device support (either local or remote memory involved in the transfer will "
        "be on the device.");
    requiresDeviceSupportOpt->default_val(false);

    bool useCuda;
    auto useCudaOpt = app.add_flag(
        "--use-cuda", useCuda, "Use cuda memory for local buffers. This will override requiresDeviceSupport option to true.");
    useCudaOpt->default_val(false);

    int cudaDeviceId;
    auto cudaDeviceIdOpt = app.add_option("--cuda-index", cudaDeviceId, "Device index used to select the CUDA gpu.");
    cudaDeviceIdOpt->default_val(0);

    CLI11_PARSE(app, argc, argv);

    // Using CUDA for local memory requires device support!
    requiresDeviceSupport |= useCuda;

    mxlFabricsProvider mxlProvider;
    auto status = mxlFabricsProviderFromString(provider.c_str(), &mxlProvider);
    if (status != MXL_STATUS_OK)
    {
        MXL_ERROR("Failed to parse provider '{}'", provider);
        return status;
    }

    if (runAsInitiator)
    {
        MXL_INFO("Running as initiator");
        auto flowJsonPath = std::filesystem::path(domain) / (flowConf + ".mxl-flow/.json");

        std::ifstream file(flowJsonPath, std::ios::in | std::ios::binary);
        if (!file)
        {
            MXL_ERROR("Failed to open file: '{}'", std::string(flowJsonPath));
            return MXL_ERR_INVALID_ARG;
        }
        std::string flowDescriptor{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        mxl::lib::FlowParser descriptorParser{flowDescriptor};

        auto app = AppInitator{
            Config{
                   .domain = domain,
                   .descriptor = descriptorParser,
                   .node = node,
                   .service = service,
                   .provider = mxlProvider,
                   .requiresDeviceSupport = requiresDeviceSupport,
                   .useCuda = useCuda,
                   .cudaDeviceId = cudaDeviceId,
                   },
        };

        if (status = app.setup(base64::from_base64(targetInfo)); status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to setup initiator with status '{}'", static_cast<int>(status));
            return status;
        }

        if (status = app.run(); status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to run initiator with status '{}'", static_cast<int>(status));
            return status;
        }
    }
    else
    {
        MXL_INFO("Running as target");

        std::ifstream file(flowConf, std::ios::in | std::ios::binary);
        if (!file)
        {
            MXL_ERROR("Failed to open file: '{}'", flowConf);
            return MXL_ERR_INVALID_ARG;
        }
        std::string flowDescriptor{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        mxl::lib::FlowParser descriptorParser{flowDescriptor};

        auto flowId = uuids::to_string(descriptorParser.getId());

        auto app = AppTarget{
            Config{
                   .domain = domain,
                   .descriptor = descriptorParser,
                   .node = node,
                   .service = service,
                   .provider = mxlProvider,
                   .requiresDeviceSupport = requiresDeviceSupport,
                   .useCuda = useCuda,
                   .cudaDeviceId = cudaDeviceId,
                   },
        };

        if (status = app.setup(flowDescriptor); status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to setup target with status '{}'", static_cast<int>(status));
            return status;
        }

        if (status = app.printInfo(); status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to print target info with status '{}'", static_cast<int>(status));
            return status;
        }

        if (status = app.run(); status != MXL_STATUS_OK)
        {
            MXL_ERROR("Failed to run target with status '{}'", static_cast<int>(status));
            return status;
        }
    }

    return MXL_STATUS_OK;
}
