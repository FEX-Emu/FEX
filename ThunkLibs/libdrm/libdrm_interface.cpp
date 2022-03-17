#include <common/GeneratorInterface.h>

#include <xf86drm.h>

template<auto>
struct fex_gen_config {
    unsigned version = 2;
};

size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

// TODO: Should not be opaque, but has function pointer members that can't individually be annotated yet
template<> struct fex_gen_type<drmServerInfo> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<drmEventContext> : fexgen::opaque_to_guest {};
// TODO: drmDevice has a nested pointer and union members
template<> struct fex_gen_type<drmDevice> : fexgen::opaque_to_guest {};
// TODO: Has void* pointer member
template<> struct fex_gen_type<drmBuf> : fexgen::opaque_to_guest {};

// TODO: Add function pointer support
//template<> struct fex_gen_config<&drmServerInfo::debug_print> : fexgen::ptr_todo_only64 {};
//template<> struct fex_gen_config<&drmServerInfo::load_module> : fexgen::ptr_todo_only64 {};
//template<> struct fex_gen_config<&drmServerInfo::get_perms> : fexgen::ptr_todo_only64 {};

//// TODO: Stable ABI (?)
//template<> struct fex_gen_config<&drmBuf::address> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_usable_size, 0, void*> : fexgen::ptr_is_untyped_address {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_free_on_host, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<drmIoctl> {};
template<> struct fex_gen_param<drmIoctl, 2, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmGetHashTable> {};
template<> struct fex_gen_config<drmGetEntry> {};
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

// TODO: Has nested *anonymous* struct as member
//template<> struct fex_gen_config<drmGetStats> {};

template<> struct fex_gen_config<drmSetInterfaceVersion> {};
template<> struct fex_gen_config<drmCommandNone> {};

// TODO: Stable ABI
template<> struct fex_gen_config<drmCommandRead> {};
template<> struct fex_gen_param<drmCommandRead, 2, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmCommandWrite> {};
template<> struct fex_gen_param<drmCommandWrite, 2, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmCommandWriteRead> {};
template<> struct fex_gen_param<drmCommandWriteRead, 2, void*> : fexgen::ptr_todo_only64 {};

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
template<> struct fex_gen_param<drmAddContextTag, 2, void*> : fexgen::ptr_pointer_passthrough {};

template<> struct fex_gen_config<drmDelContextTag> {};

// TODO: Return value should be fexgen::ptr_pointer_passthrough
template<> struct fex_gen_config<drmGetContextTag> {};

template<> struct fex_gen_config<drmGetReservedContextList> {};
template<> struct fex_gen_config<drmFreeReservedContextList> {};
template<> struct fex_gen_config<drmSwitchToContext> {};
template<> struct fex_gen_config<drmDestroyContext> {};
template<> struct fex_gen_config<drmCreateDrawable> {};
template<> struct fex_gen_config<drmDestroyDrawable> {};

template<> struct fex_gen_config<drmUpdateDrawableInfo> {};
template<> struct fex_gen_param<drmUpdateDrawableInfo, 4, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmCtlInstHandler> {};
template<> struct fex_gen_config<drmCtlUninstHandler> {};
template<> struct fex_gen_config<drmSetClientCap> {};
template<> struct fex_gen_config<drmCrtcGetSequence> {};
template<> struct fex_gen_config<drmCrtcQueueSequence> {};

// TODO: Consider ptr_is_untyped_address, but not sure this works for output pointers
template<> struct fex_gen_config<drmMap> {};
template<> struct fex_gen_param<drmMap, 3, drmAddressPtr> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmUnmap> {};
template<> struct fex_gen_param<drmUnmap, 0, drmAddress> : fexgen::ptr_todo_only64 {};

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

// TODO: Parameter is a union that includes a struct that needs repacking
template<> struct fex_gen_config<drmWaitVBlank> {};
template<> struct fex_gen_param<drmWaitVBlank, 1, drmVBlankPtr> : fexgen::ptr_todo_only64, fexgen::ptr_in {};

template<> struct fex_gen_config<drmSetServerInfo> {};
template<> struct fex_gen_config<drmError> {};

// TODO: Return value is fexgen::ptr_is_untyped_address
template<> struct fex_gen_config<drmMalloc> {};

template<> struct fex_gen_config<drmFree> {};
template<> struct fex_gen_param<drmFree, 0, void*> : fexgen::ptr_is_untyped_address {};

// TODO: These take pointers to an unnamed opaque type (i.e. void*)
// NOTE: the "value" parameters actually seem to be uintptr_t values that are cast to void*.
template<> struct fex_gen_config<drmHashCreate> {}; // TODO: Return value is opaque

template<> struct fex_gen_config<drmHashDestroy> {};
template<> struct fex_gen_param<drmHashDestroy, 0, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmHashLookup> {};
template<> struct fex_gen_param<drmHashLookup, 0, void*> : fexgen::ptr_todo_only64 {};
// TODO: Actually a pointer to a guest-uintptr_t
template<> struct fex_gen_param<drmHashLookup, 2, void**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmHashInsert> {};
template<> struct fex_gen_param<drmHashInsert, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmHashInsert, 2, void*> : fexgen::ptr_pointer_passthrough {};

template<> struct fex_gen_config<drmHashDelete> {};
template<> struct fex_gen_param<drmHashDelete, 0, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmHashFirst> {};
template<> struct fex_gen_param<drmHashFirst, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmHashFirst, 1, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmHashFirst, 2, void**> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_config<drmHashNext> {};
template<> struct fex_gen_param<drmHashNext, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmHashNext, 1, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmHashNext, 2, void**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmRandomCreate> {}; // TODO: Return value is opaque
template<> struct fex_gen_config<drmRandomDestroy> {};
template<> struct fex_gen_param<drmRandomDestroy, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_config<drmRandom> {};
template<> struct fex_gen_param<drmRandom, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_config<drmRandomDouble> {};
template<> struct fex_gen_param<drmRandomDouble, 0, void*> : fexgen::ptr_todo_only64 {};

// TODO: Same issues as drmHash
template<> struct fex_gen_config<drmSLCreate> {};

template<> struct fex_gen_config<drmSLDestroy> {};
template<> struct fex_gen_param<drmSLDestroy, 0, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLLookup> {};
template<> struct fex_gen_param<drmSLLookup, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLLookup, 2, void**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLInsert> {};
template<> struct fex_gen_param<drmSLInsert, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLInsert, 2, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLDelete> {};
template<> struct fex_gen_param<drmSLDelete, 0, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLNext> {};
template<> struct fex_gen_param<drmSLNext, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLNext, 1, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLNext, 2, void**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLFirst> {};
template<> struct fex_gen_param<drmSLFirst, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLFirst, 1, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLFirst, 2, void**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLDump> {};
template<> struct fex_gen_param<drmSLDump, 0, void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmSLLookupNeighbors> {};
template<> struct fex_gen_param<drmSLLookupNeighbors, 0, void*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLLookupNeighbors, 2, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLLookupNeighbors, 3, void**> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLLookupNeighbors, 4, unsigned long*> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<drmSLLookupNeighbors, 5, void**> : fexgen::ptr_todo_only64 {};

// NOTE: The first parameter is actually const char* and passed to drmOpen internally (but allegedly it's not used, so mark as untyped)
template<> struct fex_gen_config<drmOpenOnce> {};
template<> struct fex_gen_param<drmOpenOnce, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<drmOpenOnceWithType> {};
template<> struct fex_gen_config<drmCloseOnce> {};
template<> struct fex_gen_config<drmSetMaster> {};
template<> struct fex_gen_config<drmDropMaster> {};
template<> struct fex_gen_config<drmIsMaster> {};
template<> struct fex_gen_config<drmHandleEvent> {};
template<> struct fex_gen_config<drmGetDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<drmGetDeviceNameFromFd2> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<drmGetNodeTypeFromFd> {};
template<> struct fex_gen_config<drmPrimeHandleToFD> {};
template<> struct fex_gen_config<drmPrimeFDToHandle> {};
template<> struct fex_gen_config<drmGetPrimaryDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<drmGetRenderDeviceNameFromFd> : fexgen::custom_guest_entrypoint {};

// TODO: Some of these read/write an array of pointers (drmDevicePtr), so they need a manual implementation for 32-bit support
template<> struct fex_gen_config<drmGetDevice> {};
template<> struct fex_gen_param<drmGetDevice, 1, drmDevicePtr*> : fexgen::ptr_out, fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<drmFreeDevice> {};
template<> struct fex_gen_param<drmFreeDevice, 0, drmDevicePtr*> : fexgen::ptr_in, fexgen::ptr_todo_only64 {};

#if 0 // TODO: Re-enable
template<> struct fex_gen_config<drmGetDevices> {};
template<> struct fex_gen_config<drmFreeDevices> {};
template<> struct fex_gen_config<drmGetDevice2> {};
template<> struct fex_gen_config<drmGetDevices2> {};
template<> struct fex_gen_config<drmDevicesEqual> {};
#endif
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
