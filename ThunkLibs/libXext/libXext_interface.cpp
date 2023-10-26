#include <common/GeneratorInterface.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xregion.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/XEVI.h>
#include <X11/extensions/Xge.h>
#include <X11/extensions/XLbx.h>
#include <X11/extensions/multibuf.h>
#include <X11/extensions/MITMisc.h>
#include <X11/extensions/security.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/sync.h>
extern "C" {
#include <X11/extensions/extutil.h>
}
#include <X11/Xtrans/Xtransint.h>

template<auto>
struct fex_gen_config {
    unsigned version = 6;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<_X11XCBPrivate> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XContextDB> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XDisplayAtoms> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XIMFilter> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XkbInfoRec> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XKeytrans> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XLockInfo> : fexgen::opaque_type {};
template<> struct fex_gen_type<_XrmHashBucketRec> : fexgen::opaque_type {};

#ifndef IS_32BIT_THUNK
// Union types
template<> struct fex_gen_type<XEvent> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<xEvent> : fexgen::assume_compatible_data_layout {};

// Linked-list types
template<> struct fex_gen_type<_XtransConnFd> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<_XQEvent> : fexgen::assume_compatible_data_layout {};

// TODO: These are largely compatible, *but* contain function pointer members that need adjustment!
template<> struct fex_gen_type<_XConnectionInfo> : fexgen::assume_compatible_data_layout {}; // Also contains linked-list pointers
template<> struct fex_gen_type<_XConnWatchInfo> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<_XDisplay> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<XExtData> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<XExtDisplayInfo> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<_XExtension> : fexgen::assume_compatible_data_layout {}; // Also contains linked-list pointers
template<> struct fex_gen_type<_XInternalAsync> : fexgen::assume_compatible_data_layout {}; // Also contains linked-list pointers

// Contains nontrivial circular pointer relationships
template<> struct fex_gen_type<Screen> : fexgen::assume_compatible_data_layout {};

// TODO: This contains a nested struct type of function pointer members
template<> struct fex_gen_type<XImage> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<XExtensionHooks> : fexgen::assume_compatible_data_layout {};
#endif

template<> struct fex_gen_config<DPMSCapable> {};
template<> struct fex_gen_config<DPMSDisable> {};
template<> struct fex_gen_config<DPMSEnable> {};
template<> struct fex_gen_config<DPMSForceLevel> {};
template<> struct fex_gen_config<DPMSGetTimeouts> {};
template<> struct fex_gen_config<DPMSGetVersion> {};
template<> struct fex_gen_config<DPMSInfo> {};
template<> struct fex_gen_config<DPMSQueryExtension> {};
template<> struct fex_gen_config<DPMSSetTimeouts> {};
template<> struct fex_gen_config<XeviGetVisualInfo> {};
template<> struct fex_gen_config<XeviQueryExtension> {};
template<> struct fex_gen_config<XeviQueryVersion> {};
template<> struct fex_gen_config<XGEQueryExtension> {};
template<> struct fex_gen_config<XGEQueryVersion> {};
template<> struct fex_gen_config<XLbxGetEventBase> {};
template<> struct fex_gen_config<XLbxQueryExtension> {};
template<> struct fex_gen_config<XLbxQueryVersion> {};
template<> struct fex_gen_config<XmbufCreateBuffers> {};
template<> struct fex_gen_config<XmbufGetBufferAttributes> {};
template<> struct fex_gen_config<XmbufGetScreenInfo> {};
template<> struct fex_gen_config<XmbufGetVersion> {};
template<> struct fex_gen_config<XmbufGetWindowAttributes> {};
template<> struct fex_gen_config<XmbufQueryExtension> {};
template<> struct fex_gen_config<XMissingExtension> {};
template<> struct fex_gen_config<XMITMiscGetBugMode> {};
template<> struct fex_gen_config<XMITMiscQueryExtension> {};
template<> struct fex_gen_config<XMITMiscSetBugMode> {};
template<> struct fex_gen_config<XSecurityQueryExtension> {};
template<> struct fex_gen_config<XSecurityRevokeAuthorization> {};
template<> struct fex_gen_config<XShapeQueryExtension> {};
template<> struct fex_gen_config<XShapeQueryExtents> {};
template<> struct fex_gen_config<XShapeQueryVersion> {};
template<> struct fex_gen_config<XShmAttach> {};
template<> struct fex_gen_config<XShmDetach> {};
template<> struct fex_gen_config<XShmGetEventBase> {};
template<> struct fex_gen_config<XShmGetImage> {};
template<> struct fex_gen_config<XShmPixmapFormat> {};
template<> struct fex_gen_config<XShmPutImage> {};
template<> struct fex_gen_config<XShmQueryExtension> {};
template<> struct fex_gen_config<XShmQueryVersion> {};
template<> struct fex_gen_config<XSyncAwait> {};
template<> struct fex_gen_config<XSyncAwaitFence> {};
template<> struct fex_gen_config<XSyncChangeAlarm> {};
template<> struct fex_gen_config<XSyncChangeCounter> {};
template<> struct fex_gen_config<XSyncDestroyAlarm> {};
template<> struct fex_gen_config<XSyncDestroyCounter> {};
template<> struct fex_gen_config<XSyncDestroyFence> {};
template<> struct fex_gen_config<XSyncGetPriority> {};
template<> struct fex_gen_config<XSyncInitialize> {};
template<> struct fex_gen_config<XSyncQueryAlarm> {};
template<> struct fex_gen_config<XSyncQueryCounter> {};
template<> struct fex_gen_config<XSyncQueryExtension> {};
template<> struct fex_gen_config<XSyncQueryFence> {};
template<> struct fex_gen_config<XSyncResetFence> {};
template<> struct fex_gen_config<XSyncSetCounter> {};
template<> struct fex_gen_config<XSyncSetPriority> {};
template<> struct fex_gen_config<XSyncTriggerFence> {};
template<> struct fex_gen_config<XSyncValueEqual> {};
template<> struct fex_gen_config<XSyncValueGreaterOrEqual> {};
template<> struct fex_gen_config<XSyncValueGreaterThan> {};
template<> struct fex_gen_config<XSyncValueHigh32> {};
template<> struct fex_gen_config<XSyncValueIsNegative> {};
template<> struct fex_gen_config<XSyncValueIsPositive> {};
template<> struct fex_gen_config<XSyncValueIsZero> {};
template<> struct fex_gen_config<XSyncValueLessOrEqual> {};
template<> struct fex_gen_config<XSyncValueLessThan> {};
template<> struct fex_gen_config<XShapeInputSelected> {};
template<> struct fex_gen_config<XShmCreatePixmap> {};
template<> struct fex_gen_config<XSyncValueLow32> {};
template<> struct fex_gen_config<XmbufChangeBufferAttributes> {};
template<> struct fex_gen_config<XmbufChangeWindowAttributes> {};
template<> struct fex_gen_config<XmbufClearBufferArea> {};
template<> struct fex_gen_config<XmbufDestroyBuffers> {};
template<> struct fex_gen_config<XmbufDisplayBuffers> {};
template<> struct fex_gen_config<XSecurityFreeXauth> {};
template<> struct fex_gen_config<XShapeCombineMask> {};
template<> struct fex_gen_config<XShapeCombineRectangles> {};
template<> struct fex_gen_config<XShapeCombineRegion> {};
template<> struct fex_gen_config<XShapeCombineShape> {};
template<> struct fex_gen_config<XShapeOffsetShape> {};
template<> struct fex_gen_config<XShapeSelectInput> {};
template<> struct fex_gen_config<XSyncFreeSystemCounterList> {};
template<> struct fex_gen_config<XSyncIntsToValue> {};
template<> struct fex_gen_config<XSyncIntToValue> {};
template<> struct fex_gen_config<XSyncMaxValue> {};
template<> struct fex_gen_config<XSyncMinValue> {};
template<> struct fex_gen_config<XSyncValueAdd> {};
template<> struct fex_gen_config<XSyncValueSubtract> {};
template<> struct fex_gen_config<XmbufCreateStereoWindow> {};
template<> struct fex_gen_config<XSecurityAllocXauth> {};
template<> struct fex_gen_config<XSecurityGenerateAuthorization> {};
template<> struct fex_gen_config<XShmCreateImage> {};
template<> struct fex_gen_config<XShapeGetRectangles> {};
template<> struct fex_gen_config<XSyncCreateAlarm> {};
template<> struct fex_gen_config<XSyncCreateCounter> {};
template<> struct fex_gen_config<XSyncCreateFence> {};
template<> struct fex_gen_config<XSyncListSystemCounters> {};
template<> struct fex_gen_config<XextCreateExtension> {};
template<> struct fex_gen_config<XextDestroyExtension> {};
template<> struct fex_gen_config<XextAddDisplay> {};
template<> struct fex_gen_config<XextRemoveDisplay> {};
template<> struct fex_gen_config<XextFindDisplay> {};
template<> struct fex_gen_config<_XGetRequest> {};
template<> struct fex_gen_config<_XFlushGCCache> {};
#if __SIZEOF_POINTER__ == 8
template<> struct fex_gen_config<_XData32> {};
template<> struct fex_gen_config<_XRead32> {};
#endif
template<> struct fex_gen_config<_XDeqAsyncHandler> {};
template<> struct fex_gen_config<_XError> {};
template<> struct fex_gen_config<_XIOError> {};
template<> struct fex_gen_config<_XAllocScratch> {};
template<> struct fex_gen_config<_XAllocTemp> {};
template<> struct fex_gen_config<_XFreeTemp> {};
template<> struct fex_gen_config<_XVIDtoVisual> {};
template<> struct fex_gen_config<_XSetLastRequestRead> {};
template<> struct fex_gen_config<_XGetHostname> {};
template<> struct fex_gen_config<_XScreenOfWindow> {};
template<> struct fex_gen_config<_XAsyncErrorHandler> {};
template<> struct fex_gen_config<_XGetAsyncReply> {};
template<> struct fex_gen_config<_XGetAsyncData> {};
template<> struct fex_gen_config<_XFlush> {};
template<> struct fex_gen_config<_XEventsQueued> {};
template<> struct fex_gen_config<_XReadEvents> {};
template<> struct fex_gen_config<_XRead> {};
template<> struct fex_gen_config<_XReadPad> {};
template<> struct fex_gen_config<_XSend> {};
template<> struct fex_gen_config<_XReply> {};
template<> struct fex_gen_config<_XEnq> {};
template<> struct fex_gen_config<_XDeq> {};
template<> struct fex_gen_config<_XUnknownWireEvent> {};
template<> struct fex_gen_config<_XUnknownWireEventCookie> {};
template<> struct fex_gen_config<_XUnknownCopyEventCookie> {};
template<> struct fex_gen_config<_XUnknownNativeEvent> {};
template<> struct fex_gen_config<_XWireToEvent> {};
template<> struct fex_gen_config<_XDefaultWireError> {};
template<> struct fex_gen_config<_XPollfdCacheInit> {};
template<> struct fex_gen_config<_XPollfdCacheAdd> {};
template<> struct fex_gen_config<_XPollfdCacheDel> {};
template<> struct fex_gen_config<_XAllocID> {};
template<> struct fex_gen_config<_XAllocIDs> {};
template<> struct fex_gen_config<_XFreeExtData> {};
template<> struct fex_gen_config<_XRegisterInternalConnection> {};
template<> struct fex_gen_config<_XUnregisterInternalConnection> {};
template<> struct fex_gen_config<_XProcessInternalConnection> {};
template<> struct fex_gen_config<_XTextHeight> {};
template<> struct fex_gen_config<_XTextHeight16> {};
template<> struct fex_gen_config<_XEventToWire> {};
template<> struct fex_gen_config<_XF86LoadQueryLocaleFont> {};
template<> struct fex_gen_config<_XProcessWindowAttributes> {};
template<> struct fex_gen_config<_XDefaultError> {};
template<> struct fex_gen_config<_XDefaultIOError> {};

#if !(X11_VERSION_MAJOR >= 1 && X11_VERSION_MINOR >= 7 && X11_VERSION_PATCH >= 0)
// Doesn't exist on older X11
extern void _XDefaultIOErrorExit(Display *dpy, void *user_data);
#endif

template<> struct fex_gen_config<_XDefaultIOErrorExit> {};

template<> struct fex_gen_config<_XSetClipRectangles> {};
template<> struct fex_gen_config<_XGetWindowAttributes> {};
template<> struct fex_gen_config<_XPutBackEvent> {};
template<> struct fex_gen_config<_XIsEventCookie> {};
template<> struct fex_gen_config<_XFreeEventCookies> {};
template<> struct fex_gen_config<_XStoreEventCookie> {};
template<> struct fex_gen_config<_XFetchEventCookie> {};
template<> struct fex_gen_config<_XCopyEventCookie> {};
template<> struct fex_gen_config<xlocaledir> {};
