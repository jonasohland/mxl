#pragma once

#include <cstddef>
#include <cstdint>
#include <bits/types/struct_iovec.h>
#include <mxl/mxl.h>
#include "mxl/flow.h"
#include "mxl/platform.h"

#define MXL_FABRICS_UNUSED(x) (void)x

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct mxlFabricsInstance_t* mxlFabricsInstance;
    typedef struct mxlFabricsTarget_t* mxlFabricsTarget;
    typedef struct mxlTargetInfo_t* mxlTargetInfo;
    typedef struct mxlFabricsInitiator_t* mxlFabricsInitiator;

    typedef enum mxlFabricsProvider
    {
        MXL_SHARING_PROVIDER_AUTO = 0,
        MXL_SHARING_PROVIDER_TCP = 1,
        MXL_SHARING_PROVIDER_VERBS = 2,
        MXL_SHARING_PROVIDER_EFA = 3,
        MXL_SHARING_PROVIDER_SHM = 4,
    } mxlFabricsProvider;

    typedef struct mxlEndpointAddress_t
    {
        char const* node;
        char const* service;
    } mxlEndpointAddress;

    typedef void* mxlRegions;

    typedef struct mxlTargetConfig_t
    {
        mxlEndpointAddress endpointAddress;
        mxlFabricsProvider provider;
        mxlRegions regions;
        bool deviceSupport;
    } mxlTargetConfig;

    typedef struct mxlInitiatorConfig_t
    {
        mxlEndpointAddress endpointAddress;
        mxlFabricsProvider provider;
        mxlRegions regions;
        bool deviceSupport;
    } mxlInitiatorConfig;

    typedef enum mxlMemoryRegionType_t // TODO:  this Might be redundant with PayloadLocation in flow.h
    {
        MXL_MEMORY_REGION_TYPE_HOST = 0,
        MXL_MEMORY_REGION_TYPE_CUDA = 1,
    } mxlMemoryRegionType;

    typedef struct mxlMemoryRegion_t
    {
        uint64_t addr;
        size_t size;
        mxlMemoryRegionType type;
    } mxlMemoryRegion;

    // A group of memory regions that should be processed/transfered together
    typedef struct mxlMemoryRegionGroup_t
    {
        mxlMemoryRegion* memoryRegions;
        size_t count;
    } mxlMemoryRegionGroup;

    typedef void (*mxlFabricsCompletionCallback_t)(uint64_t in_index, void* in_userData);

    MXL_EXPORT
    mxlStatus mxlFabricsRegionsFromFlow(mxlFlowData const in_flowData, mxlRegions* out_regions);

    MXL_EXPORT
    mxlStatus mxlFabricsRegionsFromBufferGroups(mxlMemoryRegionGroup const* in_groups, size_t in_count, mxlRegions* out_regions);

    MXL_EXPORT
    mxlStatus mxlFabricsRegionsFree(mxlRegions regions);

    /**
     * Create a new mxl-fabrics from an mxl instance. Targets and initiators created from this mxl-fabrics instance
     * will have access to the flows created in the supplied mxl instance.
     * \param in_instance An mxlInstance previously created with mxlCreateInstance().
     * \param out_fabricsInstance Returns a pointer to the created mxlFabricsInstance.
     * \return MXL_STATUS_OK if the instance was successfully created
     */
    MXL_EXPORT
    mxlStatus mxlFabricsCreateInstance(mxlInstance in_instance, mxlFabricsInstance* out_fabricsInstance);

    /**
     * Destroy a mxlFabricsInstance.
     * \param in_instance The mxlFabricsInstance to destroy.
     * \return MXL_STATUS_OK if the instances was successfully destroyed.
     */
    MXL_EXPORT
    mxlStatus mxlFabricsDestroyInstance(mxlFabricsInstance in_instance);

    /**
     * Create a fabrics target instance.
     * \param in_fabricsInstance A valid mxl fabrics instance
     * \param out_target A valid fabrics target
     */
    MXL_EXPORT
    mxlStatus mxlFabricsCreateTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget* out_target);

    /**
     * Destroy a fabrics target instance.
     * \param in_fabricsInstance A valid mxl fabrics instance
     * \param in_target A valid fabrics target
     */
    MXL_EXPORT
    mxlStatus mxlFabricsDestroyTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target);

    /**
     * Configure the target.
     * \param in_target A valid fabrics target
     * \param in_config The target configuration. This will be used to create an endpoint and register a memory region. The memory region corresponds
     * to the one that will be written to by the initiator.
     * \param out_info An mxlTargetInfo_t object which should be shared to a remote initiator which this target should receive data from.
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsTargetSetup(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, mxlTargetConfig* in_config,
        mxlTargetInfo* out_info);

    /**
     * Non-blocking accessor for a flow grain at a specific index
     * \param in_target A valid fabrics target
     * \param out_index The index of the grain that is ready, if any.
     * \param out_completed Indicates if the grain was written. No distinction is made between a partial or fully written grain, they both return true.
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsTargetTryNewGrain(mxlFabricsTarget in_target, uint64_t* out_index);

    /**
     * Blocking accessor for a flow grain at a specific index
     * \param in_target A valid fabrics target
     * \param out_index The index of the grain that is ready, if any.
     * \param in_timeoutMs How long should we wait for the grain (in milliseconds)
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsTargetWaitForNewGrain(mxlFabricsTarget in_target, uint64_t* out_index, uint16_t in_timeoutMs);

    /**
     * Set a callback function to be called everytime a new grain is available.
     * \param in_target A valid fabrics target
     * \param in_callback A callback function to be called when a new grain is available.
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsTargetSetCompletionCallback(mxlFabricsTarget in_target, mxlFabricsCompletionCallback_t callbackFn);

    /**
     * Create a fabrics initiator instance.
     * \param in_fabricsInstance A valid mxl fabrics instance
     * \param out_initiator A valid fabrics initiator
     */
    MXL_EXPORT
    mxlStatus mxlFabricsCreateInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator* out_initiator);

    /**
     * Destroy a fabrics initiator instance.
     * \param in_fabricsInstance A valid mxl fabrics instance
     * \param in_initiator A valid fabrics initiator
     */
    MXL_EXPORT
    mxlStatus mxlFabricsDestroyInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator);

    /**
     * Configure the initiator.
     * \param in_initiator A valid fabrics initiator
     * \param in_config The initiator configuration. This will be used to create an endpoint and register a memory region. The memory region
     * corresponds to the one that will be shared with targets.
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsInitiatorSetup(mxlFabricsInitiator in_initiator, mxlInitiatorConfig const* in_config);

    /**
     * Add a target to the initiator. This will allow the initiator to send data to the target.
     * \param in_initiator A valid fabrics initiator
     * \param in_targetInfo The target information. This should be the same as the one returned from "mxlFabricsTargetSetup".
     */
    MXL_EXPORT
    mxlStatus mxlFabricsInitiatorAddTarget(mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo);

    /**
     * Remove a target from the initiator.
     * \param in_initiator A valid fabrics initiator
     * \param in_targetInfo The target information. This should be the same as the one returned from "mxlFabricsTargetSetup".
     */
    MXL_EXPORT
    mxlStatus mxlFabricsInitiatorRemoveTarget(mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo);

    /**
     * Transfer of a grain to all added targets.
     * \param in_initiator A valid fabrics initiator
     * \param in_grainIndex The index of the grain to transfer.
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsInitiatorTransferGrain(mxlFabricsInitiator in_initiator, uint64_t in_grainIndex);

    // Below are helper functions

    /**
     * Convert a string to a fabrics provider.
     * \param in_string A valid string to convert
     * \param out_provider A valid fabrics provider to convert to
     * \return The result code. \see mxlStatus
     */
    MXL_EXPORT
    mxlStatus mxlFabricsProviderFromString(char const* in_string, mxlFabricsProvider* out_provider);

    /**
     * Convert a fabrics provider to a string.
     * \param in_provider A valid fabrics provider to convert
     * \param out_string A user supplied buffer of the correct size. Initially you can pass a NULL pointer to obtain the size of the string.
     * \param in_stringSize The size of the output string.
     */
    MXL_EXPORT
    mxlStatus mxlFabricsProviderToString(mxlFabricsProvider in_provider, char* out_string, size_t* in_stringSize);

    /**
     * Convert the target information to a string. This output string can be shared with a remote initiator.
     * \param in_targetInfo A valid target info to serialize
     * \param out_string A user supplied buffer of the correct size. Initially you can pass a NULL pointer to obtain the size of the string.
     * \param in_stringSize The size of the output string.
     */ 
    MXL_EXPORT
    mxlStatus mxlFabricsTargetInfoToString(mxlTargetInfo const in_targetInfo, char* out_string, size_t* in_stringSize);

    /**
     * Convert a string to a  target information.
     * \param in_string A valid string to deserialize
     * \param out_targetInfo A valid target info to deserialize to
     */
    MXL_EXPORT
    mxlStatus mxlFabricsTargetInfoFromString(char const* in_string, mxlTargetInfo* out_targetInfo);

    /**
     * Free a mxlTargetInfo object obtained from mxlFabricsTargetSetup() or mxlFabricsTargetInfoFromString().
     * \param in_info A mxlTargetInfo object
     * \return MXL_STATUS_OK if the mxlTargetInfo object was freed.
     */
    MXL_EXPORT
    mxlStatus mxlFabricsFreeTargetInfo(mxlTargetInfo in_info);

#ifdef __cplusplus
}
#endif
