#!/usr/bin/python3
from ThunkHelpers import *

lib("libX11")

# will have to peek on the source for these
# function("_XAllocScratch");
# function("_XAllocTemp");
# function("_XDeqAsyncHandler");
# function("_XEatData");
# function("_XEatDataWords");
# function("_XFlushGCCache");
# function("_XFlushIt");
# function("_XFreeTemp");
# function("_XGetAsyncReply");
# function("_XGetBitsPerPixel");
# function("_XGetRequest");
# function("_XGetScanlinePad");
# function("_Xglobal_lock");
# function("_Xglobal_lock_p");
# function("_XInitImageFuncPtrs");
# function("_XIOError");
# function("_XLockMutex_fn");
# function("_XLockMutex_fn_p");
# function("_XRead");
# function("_XReadPad");
# function("_XReply");
# function("_XSend");
# function("_XSetLastRequestRead");
# function("_Xsetlocale");
# function("_Xthread_init");
# function("_Xthread_waiter");
# function("_XUnlockMutex_fn");
# function("_XUnlockMutex_fn_p");
# function("_XVIDtoVisual");

# VArgs support needs manual work
# fn("char* XSetICValues(XIC, ...)") # TODO VARGS
# fn("void* XVaCreateNestedList(int, ...)") # TODO VARGS
# fn("char* XGetIMValues(XIM, ...)") # TODO VARGS

# Needs function pointer
# fn("int XCheckIfEvent(Display*, XEvent*, int (*)(Display*, XEvent*, XPointer), XPointer)")
# fn("int XPeekIfEvent(Display*, XEvent*, int (*)(Display*, XEvent*, XPointer), XPointer)")
# fn("int (* XSetIOErrorHandler(XIOErrorHandler))(Display*)")
# fn("int (* XSynchronize(Display*, int))(Display*)")
# fn("int XrmEnumerateDatabase(XrmDatabase, XrmNameList, XrmClassList, int, int (*)(_XrmHashBucketRec**, XrmBindingList, XrmQuarkList, XrmRepresentation*, XrmValue*, XPointer), XPointer)")

fn("char* XrmQuarkToString(XrmQuark)")
fn("int XrmCombineFileDatabase(const char*, _XrmHashBucketRec**, int)")
fn("int XrmGetResource(XrmDatabase, const char*, const char*, char**, XrmValue*)")
fn("int XrmQGetResource(XrmDatabase, XrmNameList, XrmClassList, XrmRepresentation*, XrmValue*)")
fn("int XrmQGetSearchList(XrmDatabase, XrmNameList, XrmClassList, _XrmHashBucketRec***, int)")
fn("int XrmQGetSearchResource(_XrmHashBucketRec***, XrmName, XrmClass, XrmRepresentation*, XrmValue*)")
fn("void XrmCombineDatabase(XrmDatabase, _XrmHashBucketRec**, int)")
fn("void XrmDestroyDatabase(XrmDatabase)")
fn("void XrmMergeDatabases(XrmDatabase, _XrmHashBucketRec**)")
fn("void XrmParseCommand(_XrmHashBucketRec**, XrmOptionDescList, int, const char*, int*, char**)")
fn("void XrmPutLineResource(_XrmHashBucketRec**, const char*)")
fn("void XrmPutStringResource(_XrmHashBucketRec**, const char*, const char*)")
fn("void XrmQPutResource(_XrmHashBucketRec**, XrmBindingList, XrmQuarkList, XrmRepresentation, XrmValue*)")
fn("void XrmSetDatabase(Display*, XrmDatabase)")
fn("void XrmStringToBindingQuarkList(const char*, XrmBindingList, XrmQuarkList)")
fn("_XrmHashBucketRec* XrmGetDatabase(Display*)")
fn("_XrmHashBucketRec* XrmGetFileDatabase(const char*)")
fn("_XrmHashBucketRec* XrmGetStringDatabase(const char*)")
fn("XrmQuark XrmUniqueQuark()")
fn("XrmQuark XrmStringToQuark(const char*)")
fn("XrmQuark XrmPermStringToQuark(const char*)")

fn("char* XDisplayName(const char*)")
fn("char* XDisplayString(Display*)")
fn("char* XFetchBuffer(Display*, int*, int)")
fn("char* XGetAtomName(Display*, Atom)")
fn("char* XGetDefault(Display*, const char*, const char*)")
fn("char* XKeysymToString(KeySym)")
fn("char** XListFonts(Display*, const char*, int, int*)")
fn("char* XResourceManagerString(Display*)")
fn("char* XScreenResourceString(Screen*)")
fn("Colormap XDefaultColormap(Display*, int)")
fn("Cursor XCreateFontCursor(Display*, unsigned int)")
fn("Cursor XCreateGlyphCursor(Display*, Font, Font, unsigned int, unsigned int, const XColor*, const XColor*)")
fn("Display* XDisplayOfIM(XIM)")
fn("Display* XDisplayOfScreen(Screen*)")
fn("Font XLoadFont(Display*, const char*)")
fn("GContext XGContextFromGC(GC)")
fn("int XAddConnectionWatch(Display*, XConnectionWatchProc, XPointer)")
fn("int XAddHost(Display*, XHostAddress*)")
fn("int XAllocColorCells(Display*, Colormap, int, long unsigned int*, unsigned int, long unsigned int*, unsigned int)")
fn("int XAllocColor(Display*, Colormap, XColor*)")
fn("int XAllocNamedColor(Display*, Colormap, const char*, XColor*, XColor*)")
fn("int XBell(Display*, int)")
fn("int XChangeActivePointerGrab(Display*, unsigned int, Cursor, Time)")
fn("int XChangeGC(Display*, GC, long unsigned int, XGCValues*)")
fn("int XChangeWindowAttributes(Display*, Window, long unsigned int, XSetWindowAttributes*)")
fn("int XClearArea(Display*, Window, int, int, unsigned int, unsigned int, int)")
fn("int XClearWindow(Display*, Window)")
fn("int XClipBox(Region, XRectangle*)")
fn("int XConfigureWindow(Display*, Window, unsigned int, XWindowChanges*)")
fn("int XConvertSelection(Display*, Atom, Atom, Atom, Window, Time)")
fn("int XCopyArea(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int)")
fn("int XCopyPlane(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int, long unsigned int)")
fn("int XDefaultDepth(Display*, int)")
fn("int XDeleteContext(Display*, XID, XContext)")
fn("int XDeleteProperty(Display*, Window, Atom)")
fn("int XDestroyRegion(Region)")
fn("int XDisableAccessControl(Display*)")
fn("int XDisplayKeycodes(Display*, int*, int*)")
fn("int XDrawArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int)")
fn("int XDrawArcs(Display*, Drawable, GC, XArc*, int)")
fn("int XDrawImageString(Display*, Drawable, GC, int, int, const char*, int)")
fn("int XDrawLine(Display*, Drawable, GC, int, int, int, int)")
fn("int XDrawLines(Display*, Drawable, GC, XPoint*, int, int)")
fn("int XDrawPoint(Display*, Drawable, GC, int, int)")
fn("int XDrawPoints(Display*, Drawable, GC, XPoint*, int, int)")
fn("int XDrawRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int)")
fn("int XDrawSegments(Display*, Drawable, GC, XSegment*, int)")
fn("int XDrawString(Display*, Drawable, GC, int, int, const char*, int)")
fn("int XEmptyRegion(Region)")
fn("int XEnableAccessControl(Display*)")
fn("int XEqualRegion(Region, Region)")
fn("int XFetchName(Display*, Window, char**)")
fn("int XFillArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int)")
fn("int XFillArcs(Display*, Drawable, GC, XArc*, int)")
fn("int XFillPolygon(Display*, Drawable, GC, XPoint*, int, int, int)")
fn("int XFillRectangles(Display*, Drawable, GC, XRectangle*, int)")
fn("int XFindContext(Display*, XID, XContext, char**)")
fn("int XFontsOfFontSet(XFontSet, XFontStruct***, char***)")
fn("int XFreeColors(Display*, Colormap, long unsigned int*, int, long unsigned int)")
fn("int XFreeModifiermap(XModifierKeymap*)")
fn("int XGetClassHint(Display*, Window, XClassHint*)")
fn("int XGetFontProperty(XFontStruct*, Atom, long unsigned int*)")
fn("int XGetGCValues(Display*, GC, long unsigned int, XGCValues*)")
fn("int XGetGeometry(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*)")
fn("int XGetInputFocus(Display*, Window*, int*)")
fn("int XGetRGBColormaps(Display*, Window, XStandardColormap**, int*, Atom)")
fn("int XGetWindowAttributes(Display*, Window, XWindowAttributes*)")
fn("int XGetWMClientMachine(Display*, Window, XTextProperty*)")
fn("int XGetWMName(Display*, Window, XTextProperty*)")
fn("int XGetWMNormalHints(Display*, Window, XSizeHints*, long int*)")
fn("int XGetWMProtocols(Display*, Window, Atom**, int*)")
fn("int XGrabButton(Display*, unsigned int, unsigned int, Window, int, unsigned int, int, int, Window, Cursor)")
fn("int XGrabKeyboard(Display*, Window, int, int, int, Time)")
fn("int XGrabKey(Display*, int, unsigned int, Window, int, int, int)")
fn("int XInternAtoms(Display*, char**, int, int, Atom*)")
fn("int XIntersectRegion(Region, Region, Region)")
fn("int XKillClient(Display*, XID)")
fn("int* XListDepths(Display*, int, int*)")
fn("int XLookupColor(Display*, Colormap, const char*, XColor*, XColor*)")
fn("int XLowerWindow(Display*, Window)")
fn("int XMapSubwindows(Display*, Window)")
fn("int XmbLookupString(XIC, XKeyPressedEvent*, char*, int, KeySym*, int*)")
fn("int XmbTextEscapement(XFontSet, const char*, int)")
fn("int XmbTextListToTextProperty(Display*, char**, int, XICCEncodingStyle, XTextProperty*)")
fn("int XmbTextPropertyToTextList(Display*, const XTextProperty*, char***, int*)")
fn("int XParseColor(Display*, Colormap, const char*, XColor*)")
fn("int XPutBackEvent(Display*, XEvent*)")
fn("int XQueryColor(Display*, Colormap, XColor*)")
fn("int XQueryColors(Display*, Colormap, XColor*, int)")
fn("int XRaiseWindow(Display*, Window)")
fn("int XReadBitmapFileData(const char*, unsigned int*, unsigned int*, unsigned char**, int*, int*)")
fn("int XRecolorCursor(Display*, Cursor, XColor*, XColor*)")
fn("int XRectInRegion(Region, int, int, unsigned int, unsigned int)")
fn("int XRefreshKeyboardMapping(XMappingEvent*)")
fn("int XRemoveHost(Display*, XHostAddress*)")
fn("int XReparentWindow(Display*, Window, Window, int, int)")
fn("int XRotateBuffers(Display*, int)")
fn("int XSaveContext(Display*, XID, XContext, const char*)")
fn("int XScreenNumberOfScreen(Screen*)")
fn("int XSetArcMode(Display*, GC, int)")
fn("int XSetBackground(Display*, GC, long unsigned int)")
fn("int XSetClipMask(Display*, GC, Pixmap)")
fn("int XSetClipOrigin(Display*, GC, int, int)")
fn("int XSetClipRectangles(Display*, GC, int, int, XRectangle*, int, int)")
fn("int XSetCloseDownMode(Display*, int)")
fn("int XSetCommand(Display*, Window, char**, int)")
fn("int XSetDashes(Display*, GC, int, const char*, int)")
fn("int XSetFillRule(Display*, GC, int)")
fn("int XSetFillStyle(Display*, GC, int)")
fn("int XSetFont(Display*, GC, Font)")
fn("int XSetForeground(Display*, GC, long unsigned int)")
fn("int XSetFunction(Display*, GC, int)")
fn("int XSetGraphicsExposures(Display*, GC, int)")
fn("int XSetIconSizes(Display*, Window, XIconSize*, int)")
fn("int XSetInputFocus(Display*, Window, int, Time)")
fn("int XSetLineAttributes(Display*, GC, unsigned int, int, int, int)")
fn("int XSetPlaneMask(Display*, GC, long unsigned int)")
fn("int XSetRegion(Display*, GC, Region)")
fn("int XSetSelectionOwner(Display*, Atom, Window, Time)")
fn("int XSetStipple(Display*, GC, Pixmap)")
fn("int XSetSubwindowMode(Display*, GC, int)")
fn("int XSetTile(Display*, GC, Pixmap)")
fn("int XSetTSOrigin(Display*, GC, int, int)")
fn("int XSetWindowBackground(Display*, Window, long unsigned int)")
fn("int XSetWindowBackgroundPixmap(Display*, Window, Pixmap)")
fn("int XSetWindowBorder(Display*, Window, long unsigned int)")
fn("int XSetWindowBorderPixmap(Display*, Window, Pixmap)")
fn("int XSetWindowBorderWidth(Display*, Window, unsigned int)")
fn("int XSetWMHints(Display*, Window, XWMHints*)")
fn("int XStoreBytes(Display*, const char*, int)")
fn("int XStoreColors(Display*, Colormap, XColor*, int)")
fn("int XStoreName(Display*, Window, const char*)")
fn("int XSubtractRegion(Region, Region, Region)")
fn("int XSupportsLocale()")
fn("int XTextWidth16(XFontStruct*, const XChar2b*, int)")
fn("int XTextWidth(XFontStruct*, const char*, int)")
fn("int XUngrabButton(Display*, unsigned int, unsigned int, Window)")
fn("int XUngrabKeyboard(Display*, Time)")
fn("int XUngrabKey(Display*, int, unsigned int, Window)")
fn("int XUnionRectWithRegion(XRectangle*, Region, Region)")
fn("int XUnionRegion(Region, Region, Region)")
fn("int XUnloadFont(Display*, Font)")
fn("int XUnmapSubwindows(Display*, Window)")
fn("int Xutf8TextListToTextProperty(Display*, char**, int, XICCEncodingStyle, XTextProperty*)")
fn("int Xutf8TextPropertyToTextList(Display*, const XTextProperty*, char***, int*)")
fn("int XwcLookupString(XIC, XKeyPressedEvent*, wchar_t*, int, KeySym*, int*)")
fn("int XwcTextEscapement(XFontSet, const wchar_t*, int)")
fn("int XwcTextListToTextProperty(Display*, wchar_t**, int, XICCEncodingStyle, XTextProperty*)")
fn("int XwcTextPropertyToTextList(Display*, const XTextProperty*, wchar_t***, int*)")
fn("int XWithdrawWindow(Display*, Window, int)")
fn("int XWMGeometry(Display*, int, const char*, const char*, unsigned int, XSizeHints*, int*, int*, int*, int*, int*)")
fn("KeySym* XGetKeyboardMapping(Display*, KeyCode, int, int*)")
fn("KeySym XStringToKeysym(const char*)")
fn("long int XExtendedMaxRequestSize(Display*)")
fn("long int XMaxRequestSize(Display*)")
fn("long unsigned int XNextRequest(Display*)")
fn("Pixmap XCreatePixmapFromBitmapData(Display*, Drawable, char*, unsigned int, unsigned int, long unsigned int, long unsigned int, unsigned int)")
fn("VisualID XVisualIDFromVisual(Visual*)")
fn("Visual* XDefaultVisual(Display*, int)")
fn("void XConvertCase(KeySym, KeySym*, KeySym*)")
fn("void XFreeFontSet(Display*, XFontSet)")
fn("void XFreeStringList(char**)")
fn("void XmbDrawImageString(Display*, Drawable, XFontSet, GC, int, int, const char*, int)")
fn("void XmbDrawString(Display*, Drawable, XFontSet, GC, int, int, const char*, int)")
fn("void XProcessInternalConnection(Display*, int)")
fn("void XrmInitialize()")
fn("void XSetAuthorization(char*, int, char*, int)")
fn("void XSetRGBColormaps(Display*, Window, XStandardColormap*, int, Atom)")
fn("void XSetWMIconName(Display*, Window, XTextProperty*)")
fn("void XSetWMName(Display*, Window, XTextProperty*)")
fn("void XSetWMProperties(Display*, Window, XTextProperty*, XTextProperty*, char**, int, XSizeHints*, XWMHints*, XClassHint*)")
fn("void XUnsetICFocus(XIC)")
fn("void XwcDrawImageString(Display*, Drawable, XFontSet, GC, int, int, const wchar_t*, int)")
fn("void XwcDrawString(Display*, Drawable, XFontSet, GC, int, int, const wchar_t*, int)")
fn("void XwcFreeStringList(wchar_t**)")
fn("Window XCreateSimpleWindow(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, long unsigned int, long unsigned int)")
fn("Window XDefaultRootWindow(Display*)")
fn("Window XGetSelectionOwner(Display*, Atom)")
fn("Window XRootWindow(Display*, int)")
fn("XExtCodes* XAddExtension(Display*)")
fn("XExtCodes* XInitExtension(Display*, const char*)")
fn("XFontSetExtents* XExtentsOfFontSet(XFontSet)")
fn("XFontStruct* XQueryFont(Display*, XID)")
fn("XHostAddress* XListHosts(Display*, int*, int*)")
fn("XIconSize* XAllocIconSize()")
fn("XModifierKeymap* XGetModifierMapping(Display*)")
fn("_XOC* XCreateFontSet(Display*, const char*, char***, int*, char**)")
fn("XPixmapFormatValues* XListPixmapFormats(Display*, int*)")
fn("_XRegion* XCreateRegion()")
fn("XSizeHints* XAllocSizeHints()")
fn("XStandardColormap* XAllocStandardColormap()")
fn("XVisualInfo* XGetVisualInfo(Display*, long int, XVisualInfo*, int*)")
fn("XWMHints* XGetWMHints(Display*, Window)")


#Display *XOpenDisplay(char *display_name);
function("XOpenDisplay");
returns("Display *");
arg("const char *");

#Colormap XCreateColormap(Display *display, Window w, Visual *visual, int alloc);
function("XCreateColormap");
returns("Colormap");
arg("Display *");
arg("Window");
arg("Visual *");
arg("int");

#Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, 
#unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
function("XCreateWindow");
returns("Window");
arg("Display *");
arg("Window");
arg("int")
arg("int")
arg("unsigned int")
arg("unsigned int")
arg("unsigned int")
arg("int")
arg("unsigned int")
arg("Visual *")
arg("unsigned long")
arg("XSetWindowAttributes *")

#int XMapWindow(Display *display, Window w);
function("XMapWindow");
returns("int");
arg("Display *");
arg("Window")


#
function("XChangeProperty")
returns("int")
arg("Display *")
arg("Window")
arg("Atom")
arg("Atom")
arg("int")
arg("int")
arg("const unsigned char *")
arg("int")

#
function("XCloseDisplay")
returns("int")
arg("Display *")

#
function("XDestroyWindow")
returns("int")
arg("Display *")
arg("Window")

#
function("XFree")
returns("int")
arg("void*")

#
function("XInternAtom")
returns("Atom")
arg("Display *")
arg("const char *")
arg("Bool")

#
function("XLookupKeysym")
returns("KeySym")
arg("XKeyEvent *")
arg("int")

#
function("XLookupString")
returns("int")
arg("XKeyEvent *")
arg("char *")
arg("int")
arg("KeySym *")
arg("XComposeStatus*")

#
function("XNextEvent")
returns("int")
arg("Display *")
arg("XEvent *")

#
function("XParseGeometry")
returns("int")
arg("const char *")
arg("int *")
arg("int *")
arg("unsigned int *")
arg("unsigned int *")

#
function("XPending")
returns("int")
arg("Display *")

#
function("XSetNormalHints")
returns("int")
arg("Display *")
arg("Window")
arg("XSizeHints *")

#
function("XSetStandardProperties")
returns("int")
arg("Display *")
arg("Window")
arg("const char *")
arg("const char *")
arg("Pixmap")
arg("char **")
arg("int")
arg("XSizeHints *")

########
########
########

fn("char* XGetICValues(XIC, ...)"); nothunkmap(); noforward(); nothunk()
fn("char* XGetICValues_internal(XIC, size_t, unsigned long*)"); noinit(); noload()

fn("_XIC* XCreateIC(XIM, ...)"); nothunkmap(); noforward(); nothunk()
fn("_XIC* XCreateIC_internal(XIM, size_t, unsigned long*)"); noinit(); noload()

fn("Atom XInternAtom(Display*, const char*, int)")
fn("char** XListExtensions(Display*, int*)")
fn("char* XSetLocaleModifiers(const char*)")
fn("Colormap XCreateColormap(Display*, Window, Visual*, int)")
fn("Cursor XCreatePixmapCursor(Display*, Pixmap, Pixmap, XColor*, XColor*, unsigned int, unsigned int)")
fn("Display* XOpenDisplay(const char*)")
fn("int XChangeProperty(Display*, Window, Atom, Atom, int, int, const unsigned char*, int)")
fn("int XCloseDisplay(Display*)")
fn("int XCloseIM(XIM)")
fn("int XDefineCursor(Display*, Window, Cursor)")
fn("int XDestroyWindow(Display*, Window)")
fn("int XDrawString16(Display*, Drawable, GC, int, int, const XChar2b*, int)")
fn("int XEventsQueued(Display*, int)")
fn("int XFillRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int)")
fn("int XFilterEvent(XEvent*, Window)")
fn("int XFlush(Display*)")
fn("int XFreeColormap(Display*, Colormap)")
fn("int XFreeCursor(Display*, Cursor)")
fn("int XFreeExtensionList(char**)")
fn("int XFreeFont(Display*, XFontStruct*)")
fn("int XFreeGC(Display*, GC)")
fn("int XFreePixmap(Display*, Pixmap)")
fn("int XFree(void*)")
fn("int XGetErrorDatabaseText(Display*, const char*, const char*, const char*, char*, int)")
fn("int XGetErrorText(Display*, int, char*, int)")
fn("int XGetEventData(Display*, XGenericEventCookie*)")
fn("int XGetWindowProperty(Display*, Window, Atom, long int, long int, int, Atom, Atom*, int*, long unsigned int*, long unsigned int*, unsigned char**)")
fn("int XGrabPointer(Display*, Window, int, unsigned int, int, int, Window, Cursor, Time)")
fn("int XGrabServer(Display*)")
fn("int XIconifyWindow(Display*, Window, int)")
fn("int XIfEvent(Display*, XEvent*, XIfEventFN*, XPointer)")
fn("int XInitThreads()")
fn("int XLookupString(XKeyEvent*, char*, int, KeySym*, XComposeStatus*)")
fn("int XMapRaised(Display*, Window)")
fn("int XMoveResizeWindow(Display*, Window, int, int, unsigned int, unsigned int)")
fn("int XMoveWindow(Display*, Window, int, int)")
fn("int XNextEvent(Display*, XEvent*)")
fn("int XPeekEvent(Display*, XEvent*)")
fn("int XPending(Display*)")
fn("int XQueryExtension(Display*, const char*, int*, int*, int*)")
fn("int XQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*)")
fn("int XQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*)")
fn("int XResetScreenSaver(Display*)")
fn("int XResizeWindow(Display*, Window, unsigned int, unsigned int)")
fn("int XSelectInput(Display*, Window, long int)")
fn("int XSendEvent(Display*, Window, int, long int, XEvent*)")
fn("XSetErrorHandlerFN* XSetErrorHandler(XErrorHandler)")
fn("int XSetTransientForHint(Display*, Window, Window)")
fn("int XSetWMProtocols(Display*, Window, Atom*, int)")
fn("int XSync(Display*, int)")
fn("int XTextExtents16(XFontStruct*, const XChar2b*, int, int*, int*, int*, XCharStruct*)")
fn("int XTranslateCoordinates(Display*, Window, Window, int, int, int*, int*, Window*)")
fn("int XUngrabPointer(Display*, Time)")
fn("int XUngrabServer(Display*)")
fn("int XUnmapWindow(Display*, Window)")
fn("int Xutf8LookupString(XIC, XKeyPressedEvent*, char*, int, KeySym*, int*)")
fn("int XWarpPointer(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int)")
fn("int XWindowEvent(Display*, Window, long int, XEvent*)")
fn("Pixmap XCreateBitmapFromData(Display*, Drawable, const char*, unsigned int, unsigned int)")
fn("Pixmap XCreatePixmap(Display*, Drawable, unsigned int, unsigned int, unsigned int)")
fn("void XDestroyIC(XIC)")
fn("void XFreeEventData(Display*, XGenericEventCookie*)")
fn("void XLockDisplay(Display*)")
fn("void XSetICFocus(XIC)")
fn("void XSetWMNormalHints(Display*, Window, XSizeHints*)")
fn("void XUnlockDisplay(Display*)")
fn("void Xutf8SetWMProperties(Display*, Window, const char*, const char*, char**, int, XSizeHints*, XWMHints*, XClassHint*)")
fn("Window XCreateWindow(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, long unsigned int, XSetWindowAttributes*)")
fn("XFontStruct* XLoadQueryFont(Display*, const char*)")
fn("_XGC* XCreateGC(Display*, Drawable, long unsigned int, XGCValues*)")
fn("XImage* XGetImage(Display*, Drawable, int, int, unsigned int, unsigned int, long unsigned int, int)")
fn("_XIM* XOpenIM(Display*, _XrmHashBucketRec*, char*, char*)")
fn("KeySym XkbKeycodeToKeysym(Display *, KeyCode, unsigned int, unsigned int)")
fn("Bool XCheckWindowEvent(Display *, Window, long, XEvent *)")
fn("int XDefineCursor(Display *, Window, Cursor)")
fn("int XUndefineCursor(Display *, Window)")
fn("Screen *XDefaultScreenOfDisplay( Display* )")
fn("int XDefaultScreen(Display*)")
fn("int XDisplayWidth(Display*, int)")
fn("int XMatchVisualInfo(Display*, int, int, int, XVisualInfo*)")
fn("int XPutImage(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int)")
fn("XImage* XCreateImage(Display*, Visual*, unsigned int, int, int, char*, unsigned int, unsigned int, int, int)")
fn("int XDisplayHeight(Display*, int)")


Generate()