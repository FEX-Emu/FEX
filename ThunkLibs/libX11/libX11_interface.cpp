#include <common/GeneratorInterface.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <X11/Xlibint.h>
#include <X11/XKBlib.h>

template<auto>
struct fex_gen_config {
    unsigned version = 6;
};

template<> struct fex_gen_type<Display> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XIC> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XIM> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XOC> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XOM> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XrmHashBucketRec> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XRegion> : fexgen::opaque_to_guest {};


// TODO: Should not be opaque, but has several issues right now: Has function pointer members, has XExtData pointers as members
template<> struct fex_gen_type<XExtData> : fexgen::opaque_to_guest {};
// TODO: This is part of XEvent, which is a huge union and likely repacked incorrectly...
template<> struct fex_gen_config<&XGenericEventCookie::data> : fexgen::ptr_todo_only64 {};

// TODO: Should not be opaque, but contains many function pointers (including one in a nested struct, which currently can't be annotated)
template<> struct fex_gen_type<XImage> : fexgen::opaque_to_guest {};
//template<> struct fex_gen_config<&XImage::funcs::create_image> : fexgen::ptr_todo_only64 {};

// TODO: Should not be opaque, but is a nontrivial union
template<> struct fex_gen_type<XEvent> : fexgen::opaque_to_guest {};

// TODO: Should not be opaque, but is a nontrivial union of pointers (possibly to opaque types, though?)
template<> struct fex_gen_type<XEDataObject> : fexgen::opaque_to_guest {};

template<> struct fex_gen_config<XFetchBytes> {};
template<> struct fex_gen_config<XLocaleOfIM> {};
template<> struct fex_gen_config<XLocaleOfOM> {};
template<> struct fex_gen_config<XmbResetIC> {};
template<> struct fex_gen_config<Xpermalloc> {};
template<> struct fex_gen_config<Xutf8ResetIC> {};
template<> struct fex_gen_config<XrmLocaleOfDatabase> {};
template<> struct fex_gen_config<XDisplayOfOM> {};
template<> struct fex_gen_config<XChangeKeyboardMapping> {};
template<> struct fex_gen_config<XCloseOM> {};
template<> struct fex_gen_config<XDrawText16> {};
template<> struct fex_gen_config<XDrawText> {};
template<> struct fex_gen_config<XInternalConnectionNumbers> {};
template<> struct fex_gen_config<XMaskEvent> {};
template<> struct fex_gen_config<XmbTextExtents> {};
template<> struct fex_gen_config<XmbTextPerCharExtents> {};
template<> struct fex_gen_config<_Xmbtowc> {};
template<> struct fex_gen_config<XNoOp> {};
template<> struct fex_gen_config<XOffsetRegion> {};
template<> struct fex_gen_config<XPointInRegion> {};
template<> struct fex_gen_config<XQueryBestCursor> {};
template<> struct fex_gen_config<XQueryBestSize> {};
template<> struct fex_gen_config<XQueryBestStipple> {};
template<> struct fex_gen_config<XQueryBestTile> {};
template<> struct fex_gen_config<XQueryKeymap> {};
template<> struct fex_gen_config<XQueryTextExtents16> {};
template<> struct fex_gen_config<XQueryTextExtents> {};
template<> struct fex_gen_config<XReadBitmapFile> {};
template<> struct fex_gen_config<XReconfigureWMWindow> {};
template<> struct fex_gen_config<XRegisterIMInstantiateCallback> : fexgen::callback_stub {};
template<> struct fex_gen_config<XRestackWindows> {};
template<> struct fex_gen_config<XRotateWindowProperties> {};
template<> struct fex_gen_config<XSetClassHint> {};
template<> struct fex_gen_config<XSetFontPath> {};
template<> struct fex_gen_config<XSetIconName> {};
template<> struct fex_gen_config<XSetModifierMapping> {};
template<> struct fex_gen_config<XSetPointerMapping> {};
template<> struct fex_gen_config<XSetScreenSaver> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetSizeHints> {};

template<> struct fex_gen_config<XSetState> {};
template<> struct fex_gen_config<XSetWMColormapWindows> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetZoomHints> {};

template<> struct fex_gen_config<XShrinkRegion> {};
template<> struct fex_gen_config<XStoreBuffer> {};
template<> struct fex_gen_config<XStoreColor> {};
template<> struct fex_gen_config<XStoreNamedColor> {};
template<> struct fex_gen_config<XStringListToTextProperty> {};
template<> struct fex_gen_config<XTextExtents> {};

template<> struct fex_gen_config<XTextPropertyToStringList> {};
template<> struct fex_gen_param<XTextPropertyToStringList, 1, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XUninstallColormap> {};
template<> struct fex_gen_config<XUnregisterIMInstantiateCallback> {};
template<> struct fex_gen_config<Xutf8TextEscapement> {};
template<> struct fex_gen_config<Xutf8TextExtents> {};
template<> struct fex_gen_config<Xutf8TextPerCharExtents> {};
template<> struct fex_gen_config<XwcTextExtents> {};
template<> struct fex_gen_config<XwcTextPerCharExtents> {};
template<> struct fex_gen_config<_Xwctomb> {};
template<> struct fex_gen_config<XWriteBitmapFile> {};
template<> struct fex_gen_config<XXorRegion> {};
template<> struct fex_gen_config<XKeysymToKeycode> {};
template<> struct fex_gen_config<XKeycodeToKeysym> {};
template<> struct fex_gen_config<XDisplayMotionBufferSize> {};
template<> struct fex_gen_config<XDestroyOC> {};
template<> struct fex_gen_config<XmbDrawText> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XmbSetWMProperties> {};

template<> struct fex_gen_config<XRemoveConnectionWatch> {};
template<> struct fex_gen_config<XrmPutFileDatabase> {};
template<> struct fex_gen_config<XrmPutResource> {};
template<> struct fex_gen_config<XrmQPutStringResource> {};
template<> struct fex_gen_config<XrmStringToQuarkList> {};
template<> struct fex_gen_config<XSetStandardColormap> {};
template<> struct fex_gen_config<XSetTextProperty> {};
template<> struct fex_gen_config<XSetWMClientMachine> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetWMSizeHints> {};

template<> struct fex_gen_config<Xutf8DrawImageString> {};
template<> struct fex_gen_config<Xutf8DrawString> {};
template<> struct fex_gen_config<Xutf8DrawText> {};
template<> struct fex_gen_config<XwcDrawText> {};
template<> struct fex_gen_config<XwcResetIC> {};
template<> struct fex_gen_config<XIMOfIC> {};
template<> struct fex_gen_config<XDeleteModifiermapEntry> {};
template<> struct fex_gen_config<XInsertModifiermapEntry> {};
template<> struct fex_gen_config<XNewModifiermap> {};
template<> struct fex_gen_config<XOMOfOC> {};
template<> struct fex_gen_config<XOpenOM> {};
template<> struct fex_gen_config<XPolygonRegion> {};

template<> struct fex_gen_config<XDefaultString> {};
template<> struct fex_gen_config<XAllocClassHint> {};
template<> struct fex_gen_config<XAllocWMHints> {};

template<> struct fex_gen_config<XListProperties> {};
template<> struct fex_gen_config<XBaseFontNameListOfFontSet> {};
template<> struct fex_gen_config<XGetFontPath> {};
template<> struct fex_gen_config<XListFontsWithInfo> {};
template<> struct fex_gen_config<XLocaleOfFontSet> {};
template<> struct fex_gen_config<XServerVendor> {};
template<> struct fex_gen_config<XCopyColormapAndFree> {};
template<> struct fex_gen_config<XDefaultColormapOfScreen> {};
template<> struct fex_gen_config<XListInstalledColormaps> {};
template<> struct fex_gen_config<XActivateScreenSaver> {};
template<> struct fex_gen_config<XAddHosts> {};
template<> struct fex_gen_config<XAddToExtensionList> {};
template<> struct fex_gen_config<XAddToSaveSet> {};
template<> struct fex_gen_config<XAllocColorPlanes> {};
template<> struct fex_gen_config<XAllowEvents> {};
template<> struct fex_gen_config<XAutoRepeatOff> {};
template<> struct fex_gen_config<XAutoRepeatOn> {};
template<> struct fex_gen_config<XBitmapBitOrder> {};
template<> struct fex_gen_config<XBitmapPad> {};
template<> struct fex_gen_config<XBitmapUnit> {};
template<> struct fex_gen_config<XCellsOfScreen> {};
template<> struct fex_gen_config<XChangeKeyboardControl> {};
template<> struct fex_gen_config<XChangePointerControl> {};
template<> struct fex_gen_config<XChangeSaveSet> {};
template<> struct fex_gen_config<XCheckMaskEvent> {};
template<> struct fex_gen_config<XCheckTypedEvent> {};
template<> struct fex_gen_config<XCheckTypedWindowEvent> {};
template<> struct fex_gen_config<XCirculateSubwindows> {};
template<> struct fex_gen_config<XCirculateSubwindowsDown> {};
template<> struct fex_gen_config<XCirculateSubwindowsUp> {};
template<> struct fex_gen_config<XConnectionNumber> {};
template<> struct fex_gen_config<XContextDependentDrawing> {};
template<> struct fex_gen_config<XContextualDrawing> {};
template<> struct fex_gen_config<XCopyGC> {};
template<> struct fex_gen_config<XDefaultDepthOfScreen> {};
template<> struct fex_gen_config<XDestroySubwindows> {};
template<> struct fex_gen_config<XDirectionalDependentDrawing> {};
template<> struct fex_gen_config<XDisplayCells> {};
template<> struct fex_gen_config<XDisplayHeightMM> {};
template<> struct fex_gen_config<XDisplayPlanes> {};
template<> struct fex_gen_config<XDisplayWidthMM> {};
template<> struct fex_gen_config<XDoesBackingStore> {};
template<> struct fex_gen_config<XDoesSaveUnders> {};
template<> struct fex_gen_config<XDrawImageString16> {};
template<> struct fex_gen_config<XDrawRectangles> {};
template<> struct fex_gen_config<XForceScreenSaver> {};
template<> struct fex_gen_config<XFreeFontInfo> {};
template<> struct fex_gen_config<XFreeFontNames> {};
template<> struct fex_gen_config<XFreeFontPath> {};
template<> struct fex_gen_config<XGeometry> {};
template<> struct fex_gen_config<XGetAtomNames> {};

template<> struct fex_gen_config<XGetCommand> {};
template<> struct fex_gen_param<XGetCommand, 2, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XGetIconName> {};
template<> struct fex_gen_config<XGetIconSizes> {};
template<> struct fex_gen_config<XGetKeyboardControl> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XGetNormalHints> {};

template<> struct fex_gen_config<XGetPointerControl> {};
template<> struct fex_gen_config<XGetPointerMapping> {};
template<> struct fex_gen_config<XGetScreenSaver> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XGetSizeHints> {};

template<> struct fex_gen_config<XGetStandardColormap> {};
template<> struct fex_gen_config<XGetTextProperty> {};
template<> struct fex_gen_config<XGetTransientForHint> {};
template<> struct fex_gen_config<XGetWMColormapWindows> {};
template<> struct fex_gen_config<XGetWMIconName> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XGetWMSizeHints> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XGetZoomHints> {};

template<> struct fex_gen_config<XHeightMMOfScreen> {};
template<> struct fex_gen_config<XHeightOfScreen> {};
template<> struct fex_gen_config<XImageByteOrder> {};
template<> struct fex_gen_config<XInstallColormap> {};
template<> struct fex_gen_config<XMaxCmapsOfScreen> {};
template<> struct fex_gen_config<XMinCmapsOfScreen> {};
template<> struct fex_gen_config<XPlanesOfScreen> {};
template<> struct fex_gen_config<XProtocolRevision> {};
template<> struct fex_gen_config<XProtocolVersion> {};
template<> struct fex_gen_config<XQLength> {};
template<> struct fex_gen_config<XRebindKeysym> {};
template<> struct fex_gen_config<XRemoveFromSaveSet> {};
template<> struct fex_gen_config<XRemoveHosts> {};
template<> struct fex_gen_config<XScreenCount> {};
template<> struct fex_gen_config<XSetAccessControl> {};
template<> struct fex_gen_config<XSetWindowColormap> {};
template<> struct fex_gen_config<XVendorRelease> {};
template<> struct fex_gen_config<XWidthMMOfScreen> {};
template<> struct fex_gen_config<XWidthOfScreen> {};
template<> struct fex_gen_config<XEventMaskOfScreen> {};
template<> struct fex_gen_config<XAllPlanes> {};
template<> struct fex_gen_config<XBlackPixel> {};
template<> struct fex_gen_config<XBlackPixelOfScreen> {};
template<> struct fex_gen_config<XLastKnownRequestProcessed> {};
template<> struct fex_gen_config<XWhitePixel> {};
template<> struct fex_gen_config<XWhitePixelOfScreen> {};
template<> struct fex_gen_config<XScreenOfDisplay> {};
template<> struct fex_gen_config<XDefaultVisualOfScreen> {};
template<> struct fex_gen_config<XFlushGC> {};
template<> struct fex_gen_config<XRootWindowOfScreen> {};
template<> struct fex_gen_config<XEHeadOfExtensionList> {};
template<> struct fex_gen_config<XFindOnExtensionList> {};
template<> struct fex_gen_config<XDefaultGC> {};
template<> struct fex_gen_config<XDefaultGCOfScreen> {};
template<> struct fex_gen_config<XGetSubImage> {};
template<> struct fex_gen_config<XGetMotionEvents> {};

template<> struct fex_gen_config<_XReadEvents> {};

template<> struct fex_gen_config<XInitImage> {}; // TODO: Fixup vtable for guest use
template<> struct fex_gen_config<XrmQuarkToString> {};
template<> struct fex_gen_config<XrmCombineFileDatabase> {};
template<> struct fex_gen_config<XrmGetResource> {};
template<> struct fex_gen_config<XrmQGetResource> {};

template<> struct fex_gen_config<XrmQGetSearchList> {};
template<> struct fex_gen_param<XrmQGetSearchList, 3, XrmHashTable*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XrmQGetSearchResource> {};
template<> struct fex_gen_param<XrmQGetSearchResource, 0, XrmHashTable*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XrmCombineDatabase> {};
template<> struct fex_gen_config<XrmDestroyDatabase> {};
template<> struct fex_gen_config<XrmMergeDatabases> {};
template<> struct fex_gen_config<XrmParseCommand> {};
template<> struct fex_gen_config<XrmPutLineResource> {};
template<> struct fex_gen_config<XrmPutStringResource> {};
template<> struct fex_gen_config<XrmQPutResource> {};
template<> struct fex_gen_config<XrmSetDatabase> {};
template<> struct fex_gen_config<XrmStringToBindingQuarkList> {};
template<> struct fex_gen_config<XrmGetDatabase> {};
template<> struct fex_gen_config<XrmGetFileDatabase> {};
template<> struct fex_gen_config<XrmGetStringDatabase> {};
template<> struct fex_gen_config<XrmUniqueQuark> {};
template<> struct fex_gen_config<XrmStringToQuark> {};
template<> struct fex_gen_config<XrmPermStringToQuark> {};

template<> struct fex_gen_config<XDisplayName> {};
template<> struct fex_gen_config<XDisplayString> {};
template<> struct fex_gen_config<XFetchBuffer> {};
template<> struct fex_gen_config<XGetAtomName> {};
template<> struct fex_gen_config<XGetDefault> {};
template<> struct fex_gen_config<XKeysymToString> {};
template<> struct fex_gen_config<XListFonts> {};
template<> struct fex_gen_config<XResourceManagerString> {};
template<> struct fex_gen_config<XScreenResourceString> {};
template<> struct fex_gen_config<XDefaultColormap> {};
template<> struct fex_gen_config<XCreateFontCursor> {};
template<> struct fex_gen_config<XCreateGlyphCursor> {};
template<> struct fex_gen_config<XDisplayOfIM> {};
template<> struct fex_gen_config<XDisplayOfScreen> {};
template<> struct fex_gen_config<XLoadFont> {};
template<> struct fex_gen_config<XGContextFromGC> {};
template<> struct fex_gen_config<XAddConnectionWatch> : fexgen::callback_stub {};
template<> struct fex_gen_config<XAddHost> {};
template<> struct fex_gen_config<XAllocColorCells> {};
template<> struct fex_gen_config<XAllocColor> {};
template<> struct fex_gen_config<XAllocNamedColor> {};
template<> struct fex_gen_config<XBell> {};
template<> struct fex_gen_config<XChangeActivePointerGrab> {};
template<> struct fex_gen_config<XChangeGC> {};
template<> struct fex_gen_config<XChangeWindowAttributes> {};
template<> struct fex_gen_config<XClearArea> {};
template<> struct fex_gen_config<XClearWindow> {};
template<> struct fex_gen_config<XClipBox> {};
template<> struct fex_gen_config<XConfigureWindow> {};
template<> struct fex_gen_config<XConvertSelection> {};
template<> struct fex_gen_config<XCopyArea> {};
template<> struct fex_gen_config<XCopyPlane> {};
template<> struct fex_gen_config<XDefaultDepth> {};
template<> struct fex_gen_config<XDeleteContext> {};
template<> struct fex_gen_config<XDeleteProperty> {};
template<> struct fex_gen_config<XDestroyRegion> {};
template<> struct fex_gen_config<XDisableAccessControl> {};
template<> struct fex_gen_config<XDisplayKeycodes> {};
template<> struct fex_gen_config<XDrawArc> {};
template<> struct fex_gen_config<XDrawArcs> {};
template<> struct fex_gen_config<XDrawImageString> {};
template<> struct fex_gen_config<XDrawLine> {};
template<> struct fex_gen_config<XDrawLines> {};
template<> struct fex_gen_config<XDrawPoint> {};
template<> struct fex_gen_config<XDrawPoints> {};
template<> struct fex_gen_config<XDrawRectangle> {};
template<> struct fex_gen_config<XDrawSegments> {};
template<> struct fex_gen_config<XDrawString> {};
template<> struct fex_gen_config<XEmptyRegion> {};
template<> struct fex_gen_config<XEnableAccessControl> {};
template<> struct fex_gen_config<XEqualRegion> {};
template<> struct fex_gen_config<XFetchName> {};
template<> struct fex_gen_config<XFillArc> {};
template<> struct fex_gen_config<XFillArcs> {};
template<> struct fex_gen_config<XFillPolygon> {};
template<> struct fex_gen_config<XFillRectangles> {};
template<> struct fex_gen_config<XFindContext> {};

template<> struct fex_gen_config<XFontsOfFontSet> {};
template<> struct fex_gen_param<XFontsOfFontSet, 1, XFontStruct***> : fexgen::ptr_todo_only64 {};
template<> struct fex_gen_param<XFontsOfFontSet, 2, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XFreeColors> {};
template<> struct fex_gen_config<XFreeModifiermap> {};
template<> struct fex_gen_config<XGetClassHint> {};
template<> struct fex_gen_config<XGetFontProperty> {};
template<> struct fex_gen_config<XGetGCValues> {};
template<> struct fex_gen_config<XGetGeometry> {};
template<> struct fex_gen_config<XGetInputFocus> {};
template<> struct fex_gen_config<XGetRGBColormaps> {};
template<> struct fex_gen_config<XGetWindowAttributes> {};
template<> struct fex_gen_config<XGetWMClientMachine> {};
template<> struct fex_gen_config<XGetWMName> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XGetWMNormalHints> {};

template<> struct fex_gen_config<XGetWMProtocols> {};
template<> struct fex_gen_config<XGrabButton> {};
template<> struct fex_gen_config<XGrabKeyboard> {};
template<> struct fex_gen_config<XGrabKey> {};
template<> struct fex_gen_config<XInternAtoms> {};
template<> struct fex_gen_config<XIntersectRegion> {};
template<> struct fex_gen_config<XKillClient> {};
template<> struct fex_gen_config<XListDepths> {};
template<> struct fex_gen_config<XLookupColor> {};
template<> struct fex_gen_config<XLowerWindow> {};
template<> struct fex_gen_config<XMapSubwindows> {};
template<> struct fex_gen_config<XmbLookupString> {};
template<> struct fex_gen_config<XmbTextEscapement> {};
template<> struct fex_gen_config<XmbTextListToTextProperty> {};

template<> struct fex_gen_config<XmbTextPropertyToTextList> {};
template<> struct fex_gen_param<XmbTextPropertyToTextList, 2, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XParseColor> {};
template<> struct fex_gen_config<XPutBackEvent> {};
template<> struct fex_gen_config<XQueryColor> {};
template<> struct fex_gen_config<XQueryColors> {};
template<> struct fex_gen_config<XRaiseWindow> {};
template<> struct fex_gen_config<XReadBitmapFileData> {};
template<> struct fex_gen_config<XRecolorCursor> {};
template<> struct fex_gen_config<XRectInRegion> {};
template<> struct fex_gen_config<XRefreshKeyboardMapping> {};
template<> struct fex_gen_config<XRemoveHost> {};
template<> struct fex_gen_config<XReparentWindow> {};
template<> struct fex_gen_config<XRotateBuffers> {};
template<> struct fex_gen_config<XSaveContext> {};
template<> struct fex_gen_config<XScreenNumberOfScreen> {};
template<> struct fex_gen_config<XSetArcMode> {};
template<> struct fex_gen_config<XSetBackground> {};
template<> struct fex_gen_config<XSetClipMask> {};
template<> struct fex_gen_config<XSetClipOrigin> {};
template<> struct fex_gen_config<XSetClipRectangles> {};
template<> struct fex_gen_config<XSetCloseDownMode> {};
template<> struct fex_gen_config<XSetCommand> {};
template<> struct fex_gen_config<XSetDashes> {};
template<> struct fex_gen_config<XSetFillRule> {};
template<> struct fex_gen_config<XSetFillStyle> {};
template<> struct fex_gen_config<XSetFont> {};
template<> struct fex_gen_config<XSetForeground> {};
template<> struct fex_gen_config<XSetFunction> {};
template<> struct fex_gen_config<XSetGraphicsExposures> {};
template<> struct fex_gen_config<XSetIconSizes> {};
template<> struct fex_gen_config<XSetInputFocus> {};
template<> struct fex_gen_config<XSetLineAttributes> {};
template<> struct fex_gen_config<XSetPlaneMask> {};
template<> struct fex_gen_config<XSetRegion> {};
template<> struct fex_gen_config<XSetSelectionOwner> {};
template<> struct fex_gen_config<XSetStipple> {};
template<> struct fex_gen_config<XSetSubwindowMode> {};
template<> struct fex_gen_config<XSetTile> {};
template<> struct fex_gen_config<XSetTSOrigin> {};
template<> struct fex_gen_config<XSetWindowBackground> {};
template<> struct fex_gen_config<XSetWindowBackgroundPixmap> {};
template<> struct fex_gen_config<XSetWindowBorder> {};
template<> struct fex_gen_config<XSetWindowBorderPixmap> {};
template<> struct fex_gen_config<XSetWindowBorderWidth> {};
template<> struct fex_gen_config<XSetWMHints> {};
template<> struct fex_gen_config<XStoreBytes> {};
template<> struct fex_gen_config<XStoreColors> {};
template<> struct fex_gen_config<XStoreName> {};
template<> struct fex_gen_config<XSubtractRegion> {};
template<> struct fex_gen_config<XSupportsLocale> {};
template<> struct fex_gen_config<XTextWidth16> {};
template<> struct fex_gen_config<XTextWidth> {};
template<> struct fex_gen_config<XUngrabButton> {};
template<> struct fex_gen_config<XUngrabKeyboard> {};
template<> struct fex_gen_config<XUngrabKey> {};
template<> struct fex_gen_config<XUnionRectWithRegion> {};
template<> struct fex_gen_config<XUnionRegion> {};
template<> struct fex_gen_config<XUnloadFont> {};
template<> struct fex_gen_config<XUnmapSubwindows> {};
template<> struct fex_gen_config<Xutf8TextListToTextProperty> {};

template<> struct fex_gen_config<Xutf8TextPropertyToTextList> {};
template<> struct fex_gen_param<Xutf8TextPropertyToTextList, 2, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XwcLookupString> {};
template<> struct fex_gen_config<XwcTextEscapement> {};
template<> struct fex_gen_config<XwcTextListToTextProperty> {};

template<> struct fex_gen_config<XwcTextPropertyToTextList> {};
template<> struct fex_gen_param<XwcTextPropertyToTextList, 2, wchar_t***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XWithdrawWindow> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XWMGeometry> {};

template<> struct fex_gen_config<XGetKeyboardMapping> {};
template<> struct fex_gen_config<XStringToKeysym> {};
template<> struct fex_gen_config<XExtendedMaxRequestSize> {};
template<> struct fex_gen_config<XMaxRequestSize> {};
template<> struct fex_gen_config<XNextRequest> {};
template<> struct fex_gen_config<XCreatePixmapFromBitmapData> {};
template<> struct fex_gen_config<XVisualIDFromVisual> {};
template<> struct fex_gen_config<XDefaultVisual> {};
template<> struct fex_gen_config<XConvertCase> {};
template<> struct fex_gen_config<XFreeFontSet> {};
template<> struct fex_gen_config<XFreeStringList> {};
template<> struct fex_gen_config<XmbDrawImageString> {};
template<> struct fex_gen_config<XmbDrawString> {};
template<> struct fex_gen_config<XProcessInternalConnection> {};
template<> struct fex_gen_config<XrmInitialize> {};
template<> struct fex_gen_config<XSetAuthorization> {};
template<> struct fex_gen_config<XSetRGBColormaps> {};
template<> struct fex_gen_config<XSetWMIconName> {};
template<> struct fex_gen_config<XSetWMName> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetWMProperties> {};

template<> struct fex_gen_config<XUnsetICFocus> {};
template<> struct fex_gen_config<XwcDrawImageString> {};
template<> struct fex_gen_config<XwcDrawString> {};
template<> struct fex_gen_config<XwcFreeStringList> {};
template<> struct fex_gen_config<XCreateSimpleWindow> {};
template<> struct fex_gen_config<XDefaultRootWindow> {};
template<> struct fex_gen_config<XGetSelectionOwner> {};
template<> struct fex_gen_config<XRootWindow> {};
template<> struct fex_gen_config<XAddExtension> {};
template<> struct fex_gen_config<XInitExtension> {};
template<> struct fex_gen_config<XExtentsOfFontSet> {};
template<> struct fex_gen_config<XQueryFont> {};
template<> struct fex_gen_config<XListHosts> {};
template<> struct fex_gen_config<XAllocIconSize> {};
template<> struct fex_gen_config<XGetModifierMapping> {};

template<> struct fex_gen_config<XCreateFontSet> {};
template<> struct fex_gen_param<XCreateFontSet, 2, char***> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<XListPixmapFormats> {};
template<> struct fex_gen_config<XCreateRegion> {};
template<> struct fex_gen_config<XAllocSizeHints> {};
template<> struct fex_gen_config<XAllocStandardColormap> {};
template<> struct fex_gen_config<XGetVisualInfo> {};
template<> struct fex_gen_config<XGetWMHints> {};

template<> struct fex_gen_config<XMapWindow> {};
template<> struct fex_gen_config<XLookupKeysym> {};

template<> struct fex_gen_config<XParseGeometry> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetNormalHints> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetStandardProperties> {};

template<> struct fex_gen_config<XGetICValues> {
    using uniform_va_type = unsigned long;
};

template<> struct fex_gen_config<XCreateIC> {
    using uniform_va_type = unsigned long;
};

template<> struct fex_gen_config<XIfEvent> {};
template<> struct fex_gen_config<XSetErrorHandler> : fexgen::returns_guest_pointer {};

template<> struct fex_gen_config<XInternAtom> {};
template<> struct fex_gen_config<XListExtensions> {};
template<> struct fex_gen_config<XSetLocaleModifiers> {};
template<> struct fex_gen_config<XCreateColormap> {};
template<> struct fex_gen_config<XCreatePixmapCursor> {};
template<> struct fex_gen_config<XOpenDisplay> {};
template<> struct fex_gen_config<XChangeProperty> {};
template<> struct fex_gen_config<XCloseDisplay> {};
template<> struct fex_gen_config<XCloseIM> {};
template<> struct fex_gen_config<XDestroyWindow> {};
template<> struct fex_gen_config<XDrawString16> {};
template<> struct fex_gen_config<XEventsQueued> {};
template<> struct fex_gen_config<XFillRectangle> {};
template<> struct fex_gen_config<XFilterEvent> {};
template<> struct fex_gen_config<XFlush> {};
template<> struct fex_gen_config<XFreeColormap> {};
template<> struct fex_gen_config<XFreeCursor> {};
template<> struct fex_gen_config<XFreeExtensionList> {};
template<> struct fex_gen_config<XFreeFont> {};
template<> struct fex_gen_config<XFreeGC> {};
template<> struct fex_gen_config<XFreePixmap> {};

template<> struct fex_gen_config<XFree> {};
template<> struct fex_gen_param<XFree, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<XGetErrorDatabaseText> {};
template<> struct fex_gen_config<XGetErrorText> {};
template<> struct fex_gen_config<XGetEventData> {};
template<> struct fex_gen_config<XGetWindowProperty> {};
template<> struct fex_gen_config<XGrabPointer> {};
template<> struct fex_gen_config<XGrabServer> {};
template<> struct fex_gen_config<XIconifyWindow> {};
template<> struct fex_gen_config<XInitThreads> {};
template<> struct fex_gen_config<XLookupString> {};
template<> struct fex_gen_config<XMapRaised> {};
template<> struct fex_gen_config<XMoveResizeWindow> {};
template<> struct fex_gen_config<XMoveWindow> {};
template<> struct fex_gen_config<XNextEvent> {};
template<> struct fex_gen_config<XPeekEvent> {};
template<> struct fex_gen_config<XPending> {};
template<> struct fex_gen_config<XQueryExtension> {};
template<> struct fex_gen_config<XQueryPointer> {};
template<> struct fex_gen_config<XQueryTree> {};
template<> struct fex_gen_config<XResetScreenSaver> {};
template<> struct fex_gen_config<XResizeWindow> {};
template<> struct fex_gen_config<XSelectInput> {};
template<> struct fex_gen_config<XSendEvent> {};
template<> struct fex_gen_config<XSetTransientForHint> {};
template<> struct fex_gen_config<XSetWMProtocols> {};
template<> struct fex_gen_config<XSync> {};
template<> struct fex_gen_config<XTextExtents16> {};
template<> struct fex_gen_config<XTranslateCoordinates> {};
template<> struct fex_gen_config<XUngrabPointer> {};
template<> struct fex_gen_config<XUngrabServer> {};
template<> struct fex_gen_config<XUnmapWindow> {};
template<> struct fex_gen_config<Xutf8LookupString> {};
template<> struct fex_gen_config<XWarpPointer> {};
template<> struct fex_gen_config<XWindowEvent> {};
template<> struct fex_gen_config<XCreateBitmapFromData> {};
template<> struct fex_gen_config<XCreatePixmap> {};
template<> struct fex_gen_config<XDestroyIC> {};
template<> struct fex_gen_config<XFreeEventData> {};
template<> struct fex_gen_config<XLockDisplay> {};
template<> struct fex_gen_config<XSetICFocus> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<XSetWMNormalHints> {};

template<> struct fex_gen_config<XUnlockDisplay> {};

// TODO: XSizeHints contains anonymous struct
//template<> struct fex_gen_config<Xutf8SetWMProperties> {};

template<> struct fex_gen_config<XCreateWindow> {};
template<> struct fex_gen_config<XLoadQueryFont> {};
template<> struct fex_gen_config<XCreateGC> {};
template<> struct fex_gen_config<XGetImage> {};
template<> struct fex_gen_config<XOpenIM> {};
template<> struct fex_gen_config<XkbKeycodeToKeysym> {};
template<> struct fex_gen_config<XCheckWindowEvent> {};
template<> struct fex_gen_config<XDefineCursor> {};
template<> struct fex_gen_config<XUndefineCursor> {};
template<> struct fex_gen_config<XDefaultScreenOfDisplay> {};
template<> struct fex_gen_config<XDefaultScreen> {};
template<> struct fex_gen_config<XDisplayWidth> {};
template<> struct fex_gen_config<XMatchVisualInfo> {};
template<> struct fex_gen_config<XPutImage> {};
template<> struct fex_gen_config<XCreateImage> {};
template<> struct fex_gen_config<XDisplayHeight> {};
