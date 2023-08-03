#include <common/GeneratorInterface.h>

#include <xf86drm.h>

template<auto>
struct fex_gen_config {
    unsigned version = 2;
};

template<typename>
struct fex_gen_type {};

#ifndef IS_32BIT_THUNK
// Union types with compatible data layout
template<> struct fex_gen_type<drmDevice> : fexgen::assume_compatible_data_layout {};

// Anonymous sub-structs
template<> struct fex_gen_type<drmStatsT> : fexgen::assume_compatible_data_layout {};

#endif

size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<drmIoctl> {};
template<> struct fex_gen_config<drmGetHashTable> {};
// TODO: returns struct containing a function pointer
//template<> struct fex_gen_config<drmGetEntry> {};
template<> struct fex_gen_config<drmAvailable> {};
template<> struct fex_gen_config<drmOpen> {};
template<> struct fex_gen_config<drmOpenWithType> {};
template<> struct fex_gen_config<drmOpenControl> {};
template<> struct fex_gen_config<drmOpenRender> {};
template<> struct fex_gen_config<drmClose> {};
template<> struct fex_gen_config<drmGetVersion> {};
template<> struct fex_gen_config<drmGetLibVersion> {};
template<> struct fex_gen_config<drmGetCap> {};
template<> struct fex_gen_config<drmFreeVersion> {};
template<> struct fex_gen_config<drmGetMagic> {};
template<> struct fex_gen_config<drmGetBusid> {};
template<> struct fex_gen_config<drmGetInterruptFromBusID> {};
template<> struct fex_gen_config<drmGetMap> {};
template<> struct fex_gen_config<drmGetClient> {};
template<> struct fex_gen_config<drmGetStats> {};
template<> struct fex_gen_config<drmSetInterfaceVersion> {};
template<> struct fex_gen_config<drmCommandNone> {};
template<> struct fex_gen_config<drmCommandRead> {};
template<> struct fex_gen_config<drmCommandWrite> {};
template<> struct fex_gen_config<drmCommandWriteRead> {};
template<> struct fex_gen_config<drmFreeBusid> {};
template<> struct fex_gen_config<drmSetBusid> {};
template<> struct fex_gen_config<drmAuthMagic> {};
template<> struct fex_gen_config<drmAddMap> {};
template<> struct fex_gen_config<drmRmMap> {};
template<> struct fex_gen_config<drmAddContextPrivateMapping> {};
template<> struct fex_gen_config<drmAddBufs> {};
template<> struct fex_gen_config<drmMarkBufs> {};
template<> struct fex_gen_config<drmCreateContext> {};
template<> struct fex_gen_config<drmSetContextFlags> {};
template<> struct fex_gen_config<drmGetContextFlags> {};
template<> struct fex_gen_config<drmAddContextTag> {};
template<> struct fex_gen_config<drmDelContextTag> {};
template<> struct fex_gen_config<drmGetContextTag> {};
template<> struct fex_gen_config<drmGetReservedContextList> {};
template<> struct fex_gen_config<drmFreeReservedContextList> {};
template<> struct fex_gen_config<drmSwitchToContext> {};
template<> struct fex_gen_config<drmDestroyContext> {};
template<> struct fex_gen_config<drmCreateDrawable> {};
template<> struct fex_gen_config<drmDestroyDrawable> {};
template<> struct fex_gen_config<drmUpdateDrawableInfo> {};
template<> struct fex_gen_config<drmCtlInstHandler> {};
template<> struct fex_gen_config<drmCtlUninstHandler> {};
template<> struct fex_gen_config<drmSetClientCap> {};
template<> struct fex_gen_config<drmCrtcGetSequence> {};
template<> struct fex_gen_config<drmCrtcQueueSequence> {};
template<> struct fex_gen_config<drmMap> {};
template<> struct fex_gen_config<drmUnmap> {};
template<> struct fex_gen_config<drmGetBufInfo> {};
template<> struct fex_gen_config<drmMapBufs> {};
template<> struct fex_gen_config<drmUnmapBufs> {};
template<> struct fex_gen_config<drmDMA> {};
template<> struct fex_gen_config<drmFreeBufs> {};
template<> struct fex_gen_config<drmGetLock> {};
template<> struct fex_gen_config<drmUnlock> {};
template<> struct fex_gen_config<drmFinish> {};
template<> struct fex_gen_config<drmGetContextPrivateMapping> {};
template<> struct fex_gen_config<drmScatterGatherAlloc> {};
template<> struct fex_gen_config<drmScatterGatherFree> {};
template<> struct fex_gen_config<drmWaitVBlank> {};
// TODO: Needs vtable support
//template<> struct fex_gen_config<drmSetServerInfo> {};
template<> struct fex_gen_config<drmError> {};
template<> struct fex_gen_config<drmMalloc> {};
template<> struct fex_gen_config<drmFree> {};
template<> struct fex_gen_config<drmHashCreate> {};
template<> struct fex_gen_config<drmHashDestroy> {};
template<> struct fex_gen_config<drmHashLookup> {};
template<> struct fex_gen_config<drmHashInsert> {};
template<> struct fex_gen_config<drmHashDelete> {};
template<> struct fex_gen_config<drmHashFirst> {};
template<> struct fex_gen_config<drmHashNext> {};
template<> struct fex_gen_config<drmRandomCreate> {};
template<> struct fex_gen_config<drmRandomDestroy> {};
template<> struct fex_gen_config<drmRandom> {};
template<> struct fex_gen_config<drmRandomDouble> {};
template<> struct fex_gen_config<drmSLCreate> {};
template<> struct fex_gen_config<drmSLDestroy> {};
template<> struct fex_gen_config<drmSLLookup> {};
template<> struct fex_gen_config<drmSLInsert> {};
template<> struct fex_gen_config<drmSLDelete> {};
template<> struct fex_gen_config<drmSLNext> {};
template<> struct fex_gen_config<drmSLFirst> {};
template<> struct fex_gen_config<drmSLDump> {};
template<> struct fex_gen_config<drmSLLookupNeighbors> {};
template<> struct fex_gen_config<drmOpenOnce> {};
template<> struct fex_gen_config<drmOpenOnceWithType> {};
template<> struct fex_gen_config<drmCloseOnce> {};
template<> struct fex_gen_config<drmSetMaster> {};
template<> struct fex_gen_config<drmDropMaster> {};
template<> struct fex_gen_config<drmIsMaster> {};
// TODO: Needs vtable support
//template<> struct fex_gen_config<drmHandleEvent> {};
template<> struct fex_gen_config<drmGetDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<drmGetDeviceNameFromFd2> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<drmGetNodeTypeFromFd> {};
template<> struct fex_gen_config<drmPrimeHandleToFD> {};
template<> struct fex_gen_config<drmPrimeFDToHandle> {};
template<> struct fex_gen_config<drmGetPrimaryDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<drmGetRenderDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<drmGetDevice> {};
template<> struct fex_gen_config<drmFreeDevice> {};
template<> struct fex_gen_config<drmGetDevices> {};
template<> struct fex_gen_config<drmFreeDevices> {};
template<> struct fex_gen_config<drmGetDevice2> {};
template<> struct fex_gen_config<drmGetDevices2> {};
template<> struct fex_gen_config<drmDevicesEqual> {};
template<> struct fex_gen_config<drmSyncobjCreate> {};
template<> struct fex_gen_config<drmSyncobjDestroy> {};
template<> struct fex_gen_config<drmSyncobjHandleToFD> {};
template<> struct fex_gen_config<drmSyncobjFDToHandle> {};
template<> struct fex_gen_config<drmSyncobjImportSyncFile> {};
template<> struct fex_gen_config<drmSyncobjExportSyncFile> {};
template<> struct fex_gen_config<drmSyncobjWait> {};
template<> struct fex_gen_config<drmSyncobjReset> {};
template<> struct fex_gen_config<drmSyncobjSignal> {};
template<> struct fex_gen_config<drmSyncobjTimelineSignal> {};
template<> struct fex_gen_config<drmSyncobjTimelineWait> {};
template<> struct fex_gen_config<drmSyncobjQuery> {};
template<> struct fex_gen_config<drmSyncobjQuery2> {};
template<> struct fex_gen_config<drmSyncobjTransfer> {};
