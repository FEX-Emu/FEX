// SPDX-License-Identifier: MIT
// Generated file. Be careful if modifying
#include <common/GeneratorInterface.h>
#include "cuda_defines.h"
template<auto>
struct fex_gen_config {
  unsigned version = 1;
};

template<typename>
struct fex_gen_type {};

// Type definitions
template<>
struct fex_gen_type<CUctx_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmod_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUfunc_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUlib_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUkern_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUarray_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmipmappedArray_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUtexref_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUsurfref_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUevent_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUstream_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphicsResource_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUextMemory_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUextSemaphore_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraph_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphNode_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphExec_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemPoolHandle_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUuserObject_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphDeviceUpdatableNode_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUasyncCallbackEntry_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUuuid_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemFabricHandle_st> {};
template<>
struct fex_gen_type<CUipcEventHandle_st> {};
template<>
struct fex_gen_type<CUipcMemHandle_st> {};
template<>
struct fex_gen_type<CUipcMem_flags_enum> {};
template<>
struct fex_gen_type<CUmemAttach_flags_enum> {};
template<>
struct fex_gen_type<CUctx_flags_enum> {};
template<>
struct fex_gen_type<CUevent_sched_flags_enum> {};
template<>
struct fex_gen_type<CUstream_flags_enum> {};
template<>
struct fex_gen_type<CUevent_flags_enum> {};
template<>
struct fex_gen_type<CUevent_record_flags_enum> {};
template<>
struct fex_gen_type<CUevent_wait_flags_enum> {};
template<>
struct fex_gen_type<CUstreamWaitValue_flags_enum> {};
template<>
struct fex_gen_type<CUstreamWriteValue_flags_enum> {};
template<>
struct fex_gen_type<CUstreamBatchMemOpType_enum> {};
template<>
struct fex_gen_type<CUstreamMemoryBarrier_flags_enum> {};
template<>
struct fex_gen_type<CUstreamBatchMemOpParams_union> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_BATCH_MEM_OP_NODE_PARAMS_v1_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_BATCH_MEM_OP_NODE_PARAMS_v2_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUoccupancy_flags_enum> {};
template<>
struct fex_gen_type<CUstreamUpdateCaptureDependencies_flags_enum> {};
template<>
struct fex_gen_type<CUasyncNotificationType_enum> {};
template<>
struct fex_gen_type<CUasyncNotificationInfo_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUarray_format_enum> {};
template<>
struct fex_gen_type<CUaddress_mode_enum> {};
template<>
struct fex_gen_type<CUfilter_mode_enum> {};
template<>
struct fex_gen_type<CUdevice_attribute_enum> {};
template<>
struct fex_gen_type<CUdevprop_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUpointer_attribute_enum> {};
template<>
struct fex_gen_type<CUfunction_attribute_enum> {};
template<>
struct fex_gen_type<CUfunc_cache_enum> {};
template<>
struct fex_gen_type<CUsharedconfig_enum> {};
template<>
struct fex_gen_type<CUshared_carveout_enum> {};
template<>
struct fex_gen_type<CUmemorytype_enum> {};
template<>
struct fex_gen_type<CUcomputemode_enum> {};
template<>
struct fex_gen_type<CUmem_advise_enum> {};
template<>
struct fex_gen_type<CUmem_range_attribute_enum> {};
template<>
struct fex_gen_type<CUjit_option_enum> {};
template<>
struct fex_gen_type<CUjit_target_enum> {};
template<>
struct fex_gen_type<CUjit_fallback_enum> {};
template<>
struct fex_gen_type<CUjit_cacheMode_enum> {};
template<>
struct fex_gen_type<CUjitInputType_enum> {};
template<>
struct fex_gen_type<CUlinkState_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphicsRegisterFlags_enum> {};
template<>
struct fex_gen_type<CUgraphicsMapResourceFlags_enum> {};
template<>
struct fex_gen_type<CUarray_cubemap_face_enum> {};
template<>
struct fex_gen_type<CUlimit_enum> {};
template<>
struct fex_gen_type<CUresourcetype_enum> {};
template<>
struct fex_gen_type<CUaccessProperty_enum> {};
template<>
struct fex_gen_type<CUaccessPolicyWindow_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_KERNEL_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_KERNEL_NODE_PARAMS_v2_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_KERNEL_NODE_PARAMS_v3_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEMSET_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEMSET_NODE_PARAMS_v2_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_HOST_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_HOST_NODE_PARAMS_v2_st> {};
template<>
struct fex_gen_type<CUgraphConditionalNodeType_enum> {};
template<>
struct fex_gen_type<CUgraphNodeType_enum> {};
template<>
struct fex_gen_type<CUgraphDependencyType_enum> {};
template<>
struct fex_gen_type<CUgraphEdgeData_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphInstantiateResult_enum> {};
template<>
struct fex_gen_type<CUDA_GRAPH_INSTANTIATE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUsynchronizationPolicy_enum> {};
template<>
struct fex_gen_type<CUclusterSchedulingPolicy_enum> {};
template<>
struct fex_gen_type<CUlaunchMemSyncDomain_enum> {};
template<>
struct fex_gen_type<CUlaunchMemSyncDomainMap_st> {};
template<>
struct fex_gen_type<CUlaunchAttributeID_enum> {};
template<>
struct fex_gen_type<CUlaunchAttributeValue_union> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUlaunchAttribute_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUlaunchConfig_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUstreamCaptureStatus_enum> {};
template<>
struct fex_gen_type<CUstreamCaptureMode_enum> {};
template<>
struct fex_gen_type<CUdriverProcAddress_flags_enum> {};
template<>
struct fex_gen_type<CUdriverProcAddressQueryResult_enum> {};
template<>
struct fex_gen_type<CUexecAffinityType_enum> {};
template<>
struct fex_gen_type<CUexecAffinitySmCount_st> {};
template<>
struct fex_gen_type<CUexecAffinityParam_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUlibraryOption_enum> {};
template<>
struct fex_gen_type<CUlibraryHostUniversalFunctionAndDataTable_st> {};
template<>
struct fex_gen_type<CUdevice_P2PAttribute_enum> {};
template<>
struct fex_gen_type<CUDA_MEMCPY2D_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEMCPY3D_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEMCPY3D_PEER_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEMCPY_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_ARRAY_DESCRIPTOR_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_ARRAY3D_DESCRIPTOR_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_ARRAY_SPARSE_PROPERTIES_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_ARRAY_MEMORY_REQUIREMENTS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_RESOURCE_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_TEXTURE_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUresourceViewFormat_enum> {};
template<>
struct fex_gen_type<CUDA_RESOURCE_VIEW_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUtensorMap_st> {};
template<>
struct fex_gen_type<CUtensorMapDataType_enum> {};
template<>
struct fex_gen_type<CUtensorMapInterleave_enum> {};
template<>
struct fex_gen_type<CUtensorMapSwizzle_enum> {};
template<>
struct fex_gen_type<CUtensorMapL2promotion_enum> {};
template<>
struct fex_gen_type<CUtensorMapFloatOOBfill_enum> {};
template<>
struct fex_gen_type<CUDA_POINTER_ATTRIBUTE_P2P_TOKENS_st> {};
template<>
struct fex_gen_type<CUDA_POINTER_ATTRIBUTE_ACCESS_FLAGS_enum> {};
template<>
struct fex_gen_type<CUDA_LAUNCH_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUexternalMemoryHandleType_enum> {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_MEMORY_HANDLE_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_MEMORY_BUFFER_DESC_st> {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUexternalSemaphoreHandleType_enum> {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXT_SEM_SIGNAL_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXT_SEM_SIGNAL_NODE_PARAMS_v2_st> {};
template<>
struct fex_gen_type<CUDA_EXT_SEM_WAIT_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EXT_SEM_WAIT_NODE_PARAMS_v2_st> {};
template<>
struct fex_gen_type<CUmemAllocationHandleType_enum> {};
template<>
struct fex_gen_type<CUmemAccess_flags_enum> {};
template<>
struct fex_gen_type<CUmemLocationType_enum> {};
template<>
struct fex_gen_type<CUmemAllocationType_enum> {};
template<>
struct fex_gen_type<CUmemAllocationGranularity_flags_enum> {};
template<>
struct fex_gen_type<CUmemRangeHandleType_enum> {};
template<>
struct fex_gen_type<CUarraySparseSubresourceType_enum> {};
template<>
struct fex_gen_type<CUmemOperationType_enum> {};
template<>
struct fex_gen_type<CUmemHandleType_enum> {};
template<>
struct fex_gen_type<CUarrayMapInfo_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemLocation_st> {};
template<>
struct fex_gen_type<CUmemAllocationCompType_enum> {};
template<>
struct fex_gen_type<CUmemAllocationProp_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmulticastGranularity_flags_enum> {};
template<>
struct fex_gen_type<CUmulticastObjectProp_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemAccessDesc_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphExecUpdateResult_enum> {};
template<>
struct fex_gen_type<CUgraphExecUpdateResultInfo_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemPool_attribute_enum> {};
template<>
struct fex_gen_type<CUmemPoolProps_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUmemPoolPtrExportData_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEM_ALLOC_NODE_PARAMS_v1_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEM_ALLOC_NODE_PARAMS_v2_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_MEM_FREE_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphMem_attribute_enum> {};
template<>
struct fex_gen_type<CUDA_CHILD_GRAPH_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EVENT_RECORD_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUDA_EVENT_WAIT_NODE_PARAMS_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUgraphNodeParams_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUflushGPUDirectRDMAWritesOptions_enum> {};
template<>
struct fex_gen_type<CUGPUDirectRDMAWritesOrdering_enum> {};
template<>
struct fex_gen_type<CUflushGPUDirectRDMAWritesScope_enum> {};
template<>
struct fex_gen_type<CUflushGPUDirectRDMAWritesTarget_enum> {};
template<>
struct fex_gen_type<CUgraphDebugDot_flags_enum> {};
template<>
struct fex_gen_type<CUuserObject_flags_enum> {};
template<>
struct fex_gen_type<CUuserObjectRetain_flags_enum> {};
template<>
struct fex_gen_type<CUgraphInstantiate_flags_enum> {};
template<>
struct fex_gen_type<CUdeviceNumaConfig_enum> {};
template<>
struct fex_gen_type<CUmoduleLoadingMode_enum> {};
template<>
struct fex_gen_type<CUfunctionLoadingState_enum> {};
template<>
struct fex_gen_type<CUcoredumpSettings_enum> {};
template<>
struct fex_gen_type<CUgreenCtx_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUdevResourceDesc_st> : fexgen::opaque_type {};
template<>
struct fex_gen_type<CUdevSmResource_st> {};
template<>
struct fex_gen_type<CUdevResource_st> : fexgen::opaque_type {};

// Function definitions
namespace internal {
template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable, fexgen::indirect_guest_calls {};

// Function, parameter index, parameter type [optional]
template<auto, int, typename = void>
struct fex_gen_param {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGetErrorString> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGetErrorName> {};
#endif
template<>
struct fex_gen_config<cuInit> {};
template<>
struct fex_gen_config<cuDriverGetVersion> {};
template<>
struct fex_gen_config<cuDeviceGet> {};
template<>
struct fex_gen_config<cuDeviceGetCount> {};
template<>
struct fex_gen_config<cuDeviceGetName> {};
template<>
struct fex_gen_config<cuDeviceGetUuid> {};
template<>
struct fex_gen_config<cuDeviceGetUuid_v2> {};
template<>
struct fex_gen_config<cuDeviceGetLuid> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceTotalMem_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceGetTexture1DLinearMaxWidth> {};
#endif
template<>
struct fex_gen_config<cuDeviceGetAttribute> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceGetNvSciSyncAttributes> {};
#endif
template<>
struct fex_gen_config<cuDeviceSetMemPool> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceGetMemPool> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceGetDefaultMemPool> {};
#endif
template<>
struct fex_gen_config<cuDeviceGetExecAffinitySupport> {};
template<>
struct fex_gen_config<cuFlushGPUDirectRDMAWrites> {};
template<>
struct fex_gen_config<cuDeviceGetProperties> {};
template<>
struct fex_gen_config<cuDeviceComputeCapability> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDevicePrimaryCtxRetain> {};
#endif
template<>
struct fex_gen_config<cuDevicePrimaryCtxRelease_v2> {};
template<>
struct fex_gen_config<cuDevicePrimaryCtxSetFlags_v2> {};
template<>
struct fex_gen_config<cuDevicePrimaryCtxGetState> {};
template<>
struct fex_gen_config<cuDevicePrimaryCtxReset_v2> {};
template<>
struct fex_gen_config<cuCtxCreate_v2> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<cuCtxCreate_v2, 0, CUcontext*> : fexgen::ptr_passthrough {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxCreate_v3> {};
#endif
template<>
struct fex_gen_config<cuCtxDestroy_v2> {};
template<>
struct fex_gen_config<cuCtxPushCurrent_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxPopCurrent_v2> {};
#endif
template<>
struct fex_gen_config<cuCtxSetCurrent> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxGetCurrent> {};
#endif
template<>
struct fex_gen_config<cuCtxGetDevice> {};
template<>
struct fex_gen_config<cuCtxGetFlags> {};
template<>
struct fex_gen_config<cuCtxSetFlags> {};
template<>
struct fex_gen_config<cuCtxGetId> {};
template<>
struct fex_gen_config<cuCtxSynchronize> {};
template<>
struct fex_gen_config<cuCtxSetLimit> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxGetLimit> {};
#endif
template<>
struct fex_gen_config<cuCtxGetCacheConfig> {};
template<>
struct fex_gen_config<cuCtxSetCacheConfig> {};
template<>
struct fex_gen_config<cuCtxGetApiVersion> {};
template<>
struct fex_gen_config<cuCtxGetStreamPriorityRange> {};
template<>
struct fex_gen_config<cuCtxResetPersistingL2Cache> {};
template<>
struct fex_gen_config<cuCtxGetExecAffinity> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxAttach> {};
#endif
template<>
struct fex_gen_config<cuCtxDetach> {};
template<>
struct fex_gen_config<cuCtxGetSharedMemConfig> {};
template<>
struct fex_gen_config<cuCtxSetSharedMemConfig> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleLoad> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleLoadData> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleLoadDataEx> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleLoadFatBinary> {};
#endif
template<>
struct fex_gen_config<cuModuleUnload> {};
template<>
struct fex_gen_config<cuModuleGetLoadingMode> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleGetFunction> {};
#endif
template<>
struct fex_gen_config<cuModuleGetFunctionCount> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleEnumerateFunctions> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleGetGlobal_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLinkCreate_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLinkAddData_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLinkAddFile_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLinkComplete> {};
#endif
template<>
struct fex_gen_config<cuLinkDestroy> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleGetTexRef> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuModuleGetSurfRef> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryLoadData> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryLoadFromFile> {};
#endif
template<>
struct fex_gen_config<cuLibraryUnload> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryGetKernel> {};
#endif
template<>
struct fex_gen_config<cuLibraryGetKernelCount> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryEnumerateKernels> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryGetModule> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuKernelGetFunction> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryGetGlobal> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryGetManaged> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLibraryGetUnifiedFunction> {};
#endif
template<>
struct fex_gen_config<cuKernelGetAttribute> {};
template<>
struct fex_gen_config<cuKernelSetAttribute> {};
template<>
struct fex_gen_config<cuKernelSetCacheConfig> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuKernelGetName> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuKernelGetParamInfo> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemGetInfo_v2> {};
#endif
template<>
struct fex_gen_config<cuMemAlloc_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemAllocPitch_v2> {};
#endif
template<>
struct fex_gen_config<cuMemFree_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemGetAddressRange_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemAllocHost_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemFreeHost> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemHostAlloc> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemHostGetDevicePointer_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemHostGetFlags> {};
#endif
template<>
struct fex_gen_config<cuMemAllocManaged> {};
#ifndef IS_32BIT_THUNK
// Disabled due to callbacks
// template<>
// struct fex_gen_config<cuDeviceRegisterAsyncNotification>  {};
#endif
template<>
struct fex_gen_config<cuDeviceUnregisterAsyncNotification> {};
template<>
struct fex_gen_config<cuDeviceGetByPCIBusId> {};
template<>
struct fex_gen_config<cuDeviceGetPCIBusId> {};
template<>
struct fex_gen_config<cuIpcGetEventHandle> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuIpcOpenEventHandle> {};
#endif
template<>
struct fex_gen_config<cuIpcGetMemHandle> {};
template<>
struct fex_gen_config<cuIpcOpenMemHandle_v2> {};
template<>
struct fex_gen_config<cuIpcCloseMemHandle> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemHostRegister_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemHostUnregister> {};
#endif
template<>
struct fex_gen_config<cuMemcpy> {};
template<>
struct fex_gen_config<cuMemcpyPeer> {};
template<>
struct fex_gen_config<cuMemcpyHtoD_v2> {};
template<>
struct fex_gen_param<cuMemcpyHtoD_v2, 1, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyDtoH_v2> {};
template<>
struct fex_gen_param<cuMemcpyDtoH_v2, 0, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyDtoD_v2> {};
template<>
struct fex_gen_config<cuMemcpyDtoA_v2> {};
template<>
struct fex_gen_config<cuMemcpyAtoD_v2> {};
template<>
struct fex_gen_config<cuMemcpyHtoA_v2> {};
template<>
struct fex_gen_param<cuMemcpyHtoA_v2, 2, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyAtoH_v2> {};
template<>
struct fex_gen_param<cuMemcpyAtoH_v2, 0, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyAtoA_v2> {};
template<>
struct fex_gen_config<cuMemcpy2D_v2> {};
template<>
struct fex_gen_config<cuMemcpy2DUnaligned_v2> {};
template<>
struct fex_gen_config<cuMemcpy3D_v2> {};
template<>
struct fex_gen_config<cuMemcpy3DPeer> {};
template<>
struct fex_gen_config<cuMemcpyAsync> {};
template<>
struct fex_gen_config<cuMemcpyPeerAsync> {};
template<>
struct fex_gen_config<cuMemcpyHtoDAsync_v2> {};
template<>
struct fex_gen_param<cuMemcpyHtoDAsync_v2, 1, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyDtoHAsync_v2> {};
template<>
struct fex_gen_param<cuMemcpyDtoHAsync_v2, 0, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyDtoDAsync_v2> {};
template<>
struct fex_gen_config<cuMemcpyHtoAAsync_v2> {};
template<>
struct fex_gen_param<cuMemcpyHtoAAsync_v2, 2, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpyAtoHAsync_v2> {};
template<>
struct fex_gen_param<cuMemcpyAtoHAsync_v2, 0, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<cuMemcpy2DAsync_v2> {};
template<>
struct fex_gen_config<cuMemcpy3DAsync_v2> {};
template<>
struct fex_gen_config<cuMemcpy3DPeerAsync> {};
template<>
struct fex_gen_config<cuMemsetD8_v2> {};
template<>
struct fex_gen_config<cuMemsetD16_v2> {};
template<>
struct fex_gen_config<cuMemsetD32_v2> {};
template<>
struct fex_gen_config<cuMemsetD2D8_v2> {};
template<>
struct fex_gen_config<cuMemsetD2D16_v2> {};
template<>
struct fex_gen_config<cuMemsetD2D32_v2> {};
template<>
struct fex_gen_config<cuMemsetD8Async> {};
template<>
struct fex_gen_config<cuMemsetD16Async> {};
template<>
struct fex_gen_config<cuMemsetD32Async> {};
template<>
struct fex_gen_config<cuMemsetD2D8Async> {};
template<>
struct fex_gen_config<cuMemsetD2D16Async> {};
template<>
struct fex_gen_config<cuMemsetD2D32Async> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuArrayCreate_v2> {};
#endif
template<>
struct fex_gen_config<cuArrayGetDescriptor_v2> {};
template<>
struct fex_gen_config<cuArrayGetSparseProperties> {};
template<>
struct fex_gen_config<cuMipmappedArrayGetSparseProperties> {};
template<>
struct fex_gen_config<cuArrayGetMemoryRequirements> {};
template<>
struct fex_gen_config<cuMipmappedArrayGetMemoryRequirements> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuArrayGetPlane> {};
#endif
template<>
struct fex_gen_config<cuArrayDestroy> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuArray3DCreate_v2> {};
#endif
template<>
struct fex_gen_config<cuArray3DGetDescriptor_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMipmappedArrayCreate> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMipmappedArrayGetLevel> {};
#endif
template<>
struct fex_gen_config<cuMipmappedArrayDestroy> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemGetHandleForAddressRange> {};
#endif
template<>
struct fex_gen_config<cuMemAddressReserve> {};
template<>
struct fex_gen_config<cuMemAddressFree> {};
template<>
struct fex_gen_config<cuMemCreate> {};
template<>
struct fex_gen_config<cuMemRelease> {};
template<>
struct fex_gen_config<cuMemMap> {};
template<>
struct fex_gen_config<cuMemMapArrayAsync> {};
template<>
struct fex_gen_config<cuMemUnmap> {};
template<>
struct fex_gen_config<cuMemSetAccess> {};
template<>
struct fex_gen_config<cuMemGetAccess> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemExportToShareableHandle> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemImportFromShareableHandle> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemGetAllocationGranularity> {};
#endif
template<>
struct fex_gen_config<cuMemGetAllocationPropertiesFromHandle> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemRetainAllocationHandle> {};
#endif
template<>
struct fex_gen_config<cuMemFreeAsync> {};
template<>
struct fex_gen_config<cuMemAllocAsync> {};
template<>
struct fex_gen_config<cuMemPoolTrimTo> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemPoolSetAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemPoolGetAttribute> {};
#endif
template<>
struct fex_gen_config<cuMemPoolSetAccess> {};
template<>
struct fex_gen_config<cuMemPoolGetAccess> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemPoolCreate> {};
#endif
template<>
struct fex_gen_config<cuMemPoolDestroy> {};
template<>
struct fex_gen_config<cuMemAllocFromPoolAsync> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemPoolExportToShareableHandle> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemPoolImportFromShareableHandle> {};
#endif
template<>
struct fex_gen_config<cuMemPoolExportPointer> {};
template<>
struct fex_gen_config<cuMemPoolImportPointer> {};
template<>
struct fex_gen_config<cuMulticastCreate> {};
template<>
struct fex_gen_config<cuMulticastAddDevice> {};
template<>
struct fex_gen_config<cuMulticastBindMem> {};
template<>
struct fex_gen_config<cuMulticastBindAddr> {};
template<>
struct fex_gen_config<cuMulticastUnbind> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMulticastGetGranularity> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuPointerGetAttribute> {};
#endif
template<>
struct fex_gen_config<cuMemPrefetchAsync> {};
template<>
struct fex_gen_config<cuMemPrefetchAsync_v2> {};
template<>
struct fex_gen_config<cuMemAdvise> {};
template<>
struct fex_gen_config<cuMemAdvise_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemRangeGetAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuMemRangeGetAttributes> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuPointerSetAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuPointerGetAttributes> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamCreate> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamCreateWithPriority> {};
#endif
template<>
struct fex_gen_config<cuStreamGetPriority> {};
template<>
struct fex_gen_config<cuStreamGetFlags> {};
template<>
struct fex_gen_config<cuStreamGetId> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamGetCtx> {};
#endif
template<>
struct fex_gen_config<cuStreamWaitEvent> {};
// Disabled due to callbacks
// template<>
// struct fex_gen_config<cuStreamAddCallback>  {};
template<>
struct fex_gen_config<cuStreamBeginCapture_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamBeginCaptureToGraph> {};
#endif
template<>
struct fex_gen_config<cuThreadExchangeStreamCaptureMode> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamEndCapture> {};
#endif
template<>
struct fex_gen_config<cuStreamIsCapturing> {};
#ifndef IS_32BIT_THUNK
// template<>
// struct fex_gen_config<cuStreamGetCaptureInfo_v2>  {};
#endif
#ifndef IS_32BIT_THUNK
// template<>
// struct fex_gen_config<cuStreamGetCaptureInfo_v3>  {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamUpdateCaptureDependencies> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamUpdateCaptureDependencies_v2> {};
#endif
template<>
struct fex_gen_config<cuStreamAttachMemAsync> {};
template<>
struct fex_gen_config<cuStreamQuery> {};
template<>
struct fex_gen_config<cuStreamSynchronize> {};
template<>
struct fex_gen_config<cuStreamDestroy_v2> {};
template<>
struct fex_gen_config<cuStreamCopyAttributes> {};
template<>
struct fex_gen_config<cuStreamGetAttribute> {};
template<>
struct fex_gen_config<cuStreamSetAttribute> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuEventCreate> {};
#endif
template<>
struct fex_gen_config<cuEventRecord> {};
template<>
struct fex_gen_config<cuEventRecordWithFlags> {};
template<>
struct fex_gen_config<cuEventQuery> {};
template<>
struct fex_gen_config<cuEventSynchronize> {};
template<>
struct fex_gen_config<cuEventDestroy_v2> {};
template<>
struct fex_gen_config<cuEventElapsedTime> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuImportExternalMemory> {};
#endif
template<>
struct fex_gen_config<cuExternalMemoryGetMappedBuffer> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuExternalMemoryGetMappedMipmappedArray> {};
#endif
template<>
struct fex_gen_config<cuDestroyExternalMemory> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuImportExternalSemaphore> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuSignalExternalSemaphoresAsync> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuWaitExternalSemaphoresAsync> {};
#endif
template<>
struct fex_gen_config<cuDestroyExternalSemaphore> {};
template<>
struct fex_gen_config<cuStreamWaitValue32_v2> {};
template<>
struct fex_gen_config<cuStreamWaitValue64_v2> {};
template<>
struct fex_gen_config<cuStreamWriteValue32_v2> {};
template<>
struct fex_gen_config<cuStreamWriteValue64_v2> {};
template<>
struct fex_gen_config<cuStreamBatchMemOp_v2> {};
template<>
struct fex_gen_config<cuFuncGetAttribute> {};
template<>
struct fex_gen_config<cuFuncSetAttribute> {};
template<>
struct fex_gen_config<cuFuncSetCacheConfig> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuFuncGetModule> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuFuncGetName> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuFuncGetParamInfo> {};
#endif
template<>
struct fex_gen_config<cuFuncIsLoaded> {};
template<>
struct fex_gen_config<cuFuncLoad> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLaunchKernel> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLaunchKernelEx> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLaunchCooperativeKernel> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLaunchCooperativeKernelMultiDevice> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuLaunchHostFunc> {};
#endif
template<>
struct fex_gen_config<cuFuncSetBlockShape> {};
template<>
struct fex_gen_config<cuFuncSetSharedSize> {};
template<>
struct fex_gen_config<cuParamSetSize> {};
template<>
struct fex_gen_config<cuParamSeti> {};
template<>
struct fex_gen_config<cuParamSetf> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuParamSetv> {};
#endif
template<>
struct fex_gen_config<cuLaunch> {};
template<>
struct fex_gen_config<cuLaunchGrid> {};
template<>
struct fex_gen_config<cuLaunchGridAsync> {};
template<>
struct fex_gen_config<cuParamSetTexRef> {};
template<>
struct fex_gen_config<cuFuncSetSharedMemConfig> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphCreate> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddKernelNode_v2> {};
#endif
template<>
struct fex_gen_config<cuGraphKernelNodeGetParams_v2> {};
template<>
struct fex_gen_config<cuGraphKernelNodeSetParams_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddMemcpyNode> {};
#endif
template<>
struct fex_gen_config<cuGraphMemcpyNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphMemcpyNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddMemsetNode> {};
#endif
template<>
struct fex_gen_config<cuGraphMemsetNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphMemsetNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddHostNode> {};
#endif
template<>
struct fex_gen_config<cuGraphHostNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphHostNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddChildGraphNode> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphChildGraphNodeGetGraph> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddEmptyNode> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddEventRecordNode> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphEventRecordNodeGetEvent> {};
#endif
template<>
struct fex_gen_config<cuGraphEventRecordNodeSetEvent> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddEventWaitNode> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphEventWaitNodeGetEvent> {};
#endif
template<>
struct fex_gen_config<cuGraphEventWaitNodeSetEvent> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddExternalSemaphoresSignalNode> {};
#endif
template<>
struct fex_gen_config<cuGraphExternalSemaphoresSignalNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphExternalSemaphoresSignalNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddExternalSemaphoresWaitNode> {};
#endif
template<>
struct fex_gen_config<cuGraphExternalSemaphoresWaitNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphExternalSemaphoresWaitNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddBatchMemOpNode> {};
#endif
template<>
struct fex_gen_config<cuGraphBatchMemOpNodeGetParams> {};
template<>
struct fex_gen_config<cuGraphBatchMemOpNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecBatchMemOpNodeSetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddMemAllocNode> {};
#endif
template<>
struct fex_gen_config<cuGraphMemAllocNodeGetParams> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddMemFreeNode> {};
#endif
template<>
struct fex_gen_config<cuGraphMemFreeNodeGetParams> {};
template<>
struct fex_gen_config<cuDeviceGraphMemTrim> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceGetGraphMemAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDeviceSetGraphMemAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphClone> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphNodeFindInClone> {};
#endif
template<>
struct fex_gen_config<cuGraphNodeGetType> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphGetNodes> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphGetRootNodes> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphGetEdges> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphGetEdges_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphNodeGetDependencies> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphNodeGetDependencies_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphNodeGetDependentNodes> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphNodeGetDependentNodes_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddDependencies> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddDependencies_v2> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphRemoveDependencies> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphRemoveDependencies_v2> {};
#endif
template<>
struct fex_gen_config<cuGraphDestroyNode> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphInstantiateWithFlags> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphInstantiateWithParams> {};
#endif
template<>
struct fex_gen_config<cuGraphExecGetFlags> {};
template<>
struct fex_gen_config<cuGraphExecKernelNodeSetParams_v2> {};
template<>
struct fex_gen_config<cuGraphExecMemcpyNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecMemsetNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecHostNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecChildGraphNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecEventRecordNodeSetEvent> {};
template<>
struct fex_gen_config<cuGraphExecEventWaitNodeSetEvent> {};
template<>
struct fex_gen_config<cuGraphExecExternalSemaphoresSignalNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecExternalSemaphoresWaitNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphNodeSetEnabled> {};
template<>
struct fex_gen_config<cuGraphNodeGetEnabled> {};
template<>
struct fex_gen_config<cuGraphUpload> {};
template<>
struct fex_gen_config<cuGraphLaunch> {};
template<>
struct fex_gen_config<cuGraphExecDestroy> {};
template<>
struct fex_gen_config<cuGraphDestroy> {};
template<>
struct fex_gen_config<cuGraphExecUpdate_v2> {};
template<>
struct fex_gen_config<cuGraphKernelNodeCopyAttributes> {};
template<>
struct fex_gen_config<cuGraphKernelNodeGetAttribute> {};
template<>
struct fex_gen_config<cuGraphKernelNodeSetAttribute> {};
template<>
struct fex_gen_config<cuGraphDebugDotPrint> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuUserObjectCreate> {};
#endif
template<>
struct fex_gen_config<cuUserObjectRetain> {};
template<>
struct fex_gen_config<cuUserObjectRelease> {};
template<>
struct fex_gen_config<cuGraphRetainUserObject> {};
template<>
struct fex_gen_config<cuGraphReleaseUserObject> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddNode> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphAddNode_v2> {};
#endif
template<>
struct fex_gen_config<cuGraphNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphExecNodeSetParams> {};
template<>
struct fex_gen_config<cuGraphConditionalHandleCreate> {};
template<>
struct fex_gen_config<cuOccupancyMaxActiveBlocksPerMultiprocessor> {};
template<>
struct fex_gen_config<cuOccupancyMaxActiveBlocksPerMultiprocessorWithFlags> {};
// template<>
// struct fex_gen_config<cuOccupancyMaxPotentialBlockSize>  {};
// template<>
// struct fex_gen_config<cuOccupancyMaxPotentialBlockSizeWithFlags>  {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuOccupancyAvailableDynamicSMemPerBlock> {};
#endif
template<>
struct fex_gen_config<cuOccupancyMaxPotentialClusterSize> {};
template<>
struct fex_gen_config<cuOccupancyMaxActiveClusters> {};
template<>
struct fex_gen_config<cuTexRefSetArray> {};
template<>
struct fex_gen_config<cuTexRefSetMipmappedArray> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTexRefSetAddress_v2> {};
#endif
template<>
struct fex_gen_config<cuTexRefSetAddress2D_v3> {};
template<>
struct fex_gen_config<cuTexRefSetFormat> {};
template<>
struct fex_gen_config<cuTexRefSetAddressMode> {};
template<>
struct fex_gen_config<cuTexRefSetFilterMode> {};
template<>
struct fex_gen_config<cuTexRefSetMipmapFilterMode> {};
template<>
struct fex_gen_config<cuTexRefSetMipmapLevelBias> {};
template<>
struct fex_gen_config<cuTexRefSetMipmapLevelClamp> {};
template<>
struct fex_gen_config<cuTexRefSetMaxAnisotropy> {};
template<>
struct fex_gen_config<cuTexRefSetBorderColor> {};
template<>
struct fex_gen_config<cuTexRefSetFlags> {};
template<>
struct fex_gen_config<cuTexRefGetAddress_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTexRefGetArray> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTexRefGetMipmappedArray> {};
#endif
template<>
struct fex_gen_config<cuTexRefGetAddressMode> {};
template<>
struct fex_gen_config<cuTexRefGetFilterMode> {};
template<>
struct fex_gen_config<cuTexRefGetFormat> {};
template<>
struct fex_gen_config<cuTexRefGetMipmapFilterMode> {};
template<>
struct fex_gen_config<cuTexRefGetMipmapLevelBias> {};
template<>
struct fex_gen_config<cuTexRefGetMipmapLevelClamp> {};
template<>
struct fex_gen_config<cuTexRefGetMaxAnisotropy> {};
template<>
struct fex_gen_config<cuTexRefGetBorderColor> {};
template<>
struct fex_gen_config<cuTexRefGetFlags> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTexRefCreate> {};
#endif
template<>
struct fex_gen_config<cuTexRefDestroy> {};
template<>
struct fex_gen_config<cuSurfRefSetArray> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuSurfRefGetArray> {};
#endif
template<>
struct fex_gen_config<cuTexObjectCreate> {};
template<>
struct fex_gen_config<cuTexObjectDestroy> {};
template<>
struct fex_gen_config<cuTexObjectGetResourceDesc> {};
template<>
struct fex_gen_config<cuTexObjectGetTextureDesc> {};
template<>
struct fex_gen_config<cuTexObjectGetResourceViewDesc> {};
template<>
struct fex_gen_config<cuSurfObjectCreate> {};
template<>
struct fex_gen_config<cuSurfObjectDestroy> {};
template<>
struct fex_gen_config<cuSurfObjectGetResourceDesc> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTensorMapEncodeTiled> {};
#endif
// template<>
// struct fex_gen_config<cuTensorMapEncodeIm2col>  {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuTensorMapReplaceAddress> {};
#endif
template<>
struct fex_gen_config<cuDeviceCanAccessPeer> {};
template<>
struct fex_gen_config<cuCtxEnablePeerAccess> {};
template<>
struct fex_gen_config<cuCtxDisablePeerAccess> {};
template<>
struct fex_gen_config<cuDeviceGetP2PAttribute> {};
template<>
struct fex_gen_config<cuGraphicsUnregisterResource> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphicsSubResourceGetMappedArray> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphicsResourceGetMappedMipmappedArray> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphicsResourceGetMappedPointer_v2> {};
#endif
template<>
struct fex_gen_config<cuGraphicsResourceSetMapFlags_v2> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphicsMapResources> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGraphicsUnmapResources> {};
#endif
template<>
struct fex_gen_config<cuGetProcAddress_v2> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};
template<>
struct fex_gen_param<cuGetProcAddress_v2, 1, void**> : fexgen::ptr_passthrough {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCoredumpGetAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCoredumpGetAttributeGlobal> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCoredumpSetAttribute> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCoredumpSetAttributeGlobal> {};
#endif
template<>
struct fex_gen_config<cuGetExportTable> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<cuGetExportTable, 0, const void**> : fexgen::ptr_passthrough {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuGreenCtxCreate> {};
#endif
template<>
struct fex_gen_config<cuGreenCtxDestroy> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuCtxFromGreenCtx> {};
#endif
template<>
struct fex_gen_config<cuDeviceGetDevResource> {};
template<>
struct fex_gen_config<cuCtxGetDevResource> {};
template<>
struct fex_gen_config<cuGreenCtxGetDevResource> {};
template<>
struct fex_gen_config<cuDevSmResourceSplitByCount> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuDevResourceGenerateDesc> {};
#endif
template<>
struct fex_gen_config<cuGreenCtxRecordEvent> {};
template<>
struct fex_gen_config<cuGreenCtxWaitEvent> {};
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<cuStreamGetGreenCtx> {};
#endif
} // namespace internal
