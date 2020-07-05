#!/usr/bin/python3
from ThunkHelpers import *

lib("libX11")

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

Generate()