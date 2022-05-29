/*
$info$
tags: thunklibs|GL
desc: Handles glXGetProcAddress
$end_info$
*/

#include <stdio.h>
#include <cstring>
#include <cassert>
#include <cstdlib>

#include <shared_mutex>
#include <map>

#include "glincludes.inl"

#include <X11/Xmu/CloseHook.h>


#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"

//xmu brings in <X11/Xlibint.h> which defines Xmalloc
#ifndef Xmalloc
#error Xmalloc not defined
#endif


#define PACKER(name) fexfn_pack_##name
#define IMPL(name) fexfn_impl_##name

typedef void voidFunc();

#define STUB() do { errf("FGL: Stub\n"); assert(false && __FUNCTION__); } while (0)

#include "libFGL_private.h"

#if defined(TRACE)
template<typename T>
T tracer(T rv, const char *name) {
	dbgf("%s returned %p\n", name, (void*)rv);
	return rv;
}

void tracer(const char *name) {
	dbgf("%s returned\n", name);
}

#define TRACE(x) tracer(x, __FUNCTION__)
#define TRACEV(x) tracer(__FUNCTION__),x
#else
#define TRACE(x) x
#define TRACEV(x) x
#endif


static XVisualInfo *MapVisualInfoHostToGuest(Display *dpy, XVisualInfo *HostVis) {
	if (!HostVis) {
		dbgf("MapVisualInfoHostToGuest: Can't map null HostVis\n");
		return nullptr;
	}

	XVisualInfo v;
	
	// FEX_TODO("HostVis might not be same as guest XVisualInfo here")
	v.screen = HostVis->screen;
	v.visualid = HostVis->visualid;

	PACKER(fgl_XFree)(HostVis);

	int c;
	auto vguest = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask, &v, &c);

	if (c >= 1 && vguest != nullptr) {
		return vguest;
	} else {
		errf("MapVisualInfoHostToGuest: Guest XGetVisualInfo returned null\n");
		return nullptr;
	}
}

static XVisualInfo *MapVisualInfoGuestToHost(Display *dpy, XVisualInfo *GuestVis) {
	// FEX_TODO("Implement this")
	return GuestVis;
}

static GLXFBConfig *MapGLXFBConfigHostToGuest(GLXFBConfig *Host, int count) {
	if (!Host || count <= 0) {
		dbgf("MapGLXFBConfigHostToGuest: Host (%p) is null or count (%d) <= 0\n", Host, count);
		return nullptr;
	}

	auto rv = (GLXFBConfig *)Xmalloc(sizeof(GLXFBConfig) *count);

	if (!rv) {
		dbgf("MapGLXFBConfigHostToGuest: Xmalloc failed\n");
		return nullptr;
	}

	for (int i = 0; i < count; i++) {
		rv[i] = Host[i];
	}
	
	PACKER(fgl_XFree)(Host);

	return rv;
}

static GLXFBConfigSGIX *MapGLXFBConfigSGIXHostToGuest(GLXFBConfigSGIX *Host, int count) {
	if (!Host || count <= 0) {
		dbgf("MapGLXFBConfigSGIXHostToGuest: Host (%p) is null or count (%d) <= 0\n", Host, count);
		return nullptr;
	}

	auto rv = (GLXFBConfigSGIX *)Xmalloc(sizeof(GLXFBConfigSGIX) *count);

	if (!rv) {
		dbgf("MapGLXFBConfigSGIXHostToGuest: Xmalloc failed\n");
		return nullptr;
	}

	for (int i = 0; i < count; i++) {
		rv[i] = Host[i];
	}
	
	PACKER(fgl_XFree)(Host);

	return rv;
}

static int DisplayCloseCallback(Display *dpy, XPointer data)
{
	PACKER(fgl_RemoveGuestX11)(dpy);
	return 0;
}

static Display *GuestToHostX11(Display *dpy) {
	auto rv = PACKER(fgl_GuestToHostX11)(dpy);

	if (!rv) {
		XmuAddCloseDisplayHook(dpy, &DisplayCloseCallback, nullptr);
		rv = PACKER(fgl_AddGuestX11)(dpy, XDisplayString(dpy));

		if (!rv) {
			errf("GuestToHostX11: fgl_AddGuestX11 failed\n");
		}
	}
	return rv;
}

#define SYNC_GUEST_HOST() do { XSync(dpy, False); PACKER(fgl_FlushFromGuestX11)(dpy); } while(0)
#define SYNC_HOST_GUEST() do { PACKER(fgl_FlushFromGuestX11)(dpy); XSync(dpy, False); } while(0)


extern "C" {

XVisualInfo *IMPL(glXChooseVisual)( Display *dpy, int screen, int *attribList ) {
	SYNC_GUEST_HOST();
	auto rv = MapVisualInfoHostToGuest(dpy, TRACE(PACKER(glXChooseVisual)( GuestToHostX11(dpy), screen, attribList)));
	SYNC_HOST_GUEST();
	return rv;
}

GLXContext IMPL(glXCreateContext)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXCreateContext)( GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, vis), shareList, direct));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyContext)( Display *dpy, GLXContext ctx ) {
	SYNC_GUEST_HOST();
	TRACEV(PACKER(glXDestroyContext)(GuestToHostX11(dpy), ctx));
	SYNC_GUEST_HOST();
}

Bool IMPL(glXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXMakeCurrent)(GuestToHostX11(dpy), drawable, ctx));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask ) {
	SYNC_GUEST_HOST();
	TRACEV(PACKER(glXCopyContext)(GuestToHostX11(dpy), src, dst, mask));
	SYNC_HOST_GUEST();
}

void IMPL(glXSwapBuffers)( Display *dpy, GLXDrawable drawable ) {
	SYNC_GUEST_HOST();
	TRACEV(PACKER(glXSwapBuffers)(GuestToHostX11(dpy), drawable));
	SYNC_HOST_GUEST();
}

GLXPixmap IMPL(glXCreateGLXPixmap)( Display *dpy, XVisualInfo *visual, Pixmap pixmap ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXCreateGLXPixmap)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, visual), pixmap));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyGLXPixmap)( Display *dpy, GLXPixmap pixmap ) {
	SYNC_GUEST_HOST();
	TRACEV(PACKER(glXDestroyGLXPixmap)(GuestToHostX11(dpy), pixmap));
	SYNC_HOST_GUEST();
}

Bool IMPL(glXQueryExtension)( Display *dpy, int *errorb, int *event ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXQueryExtension)(GuestToHostX11(dpy), errorb, event));
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXQueryVersion)( Display *dpy, int *maj, int *min ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXQueryVersion)(GuestToHostX11(dpy), maj, min));
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXIsDirect)( Display *dpy, GLXContext ctx ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXIsDirect)(GuestToHostX11(dpy), ctx));
	SYNC_HOST_GUEST();
	return rv;
}

int IMPL(glXGetConfig)( Display *dpy, XVisualInfo *visual, int attrib, int *value ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(glXGetConfig)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, visual), attrib, value));
	SYNC_HOST_GUEST();
	return rv;
}

/*GLX 1.1 and later */
const char *IMPL(glXQueryExtensionsString)( Display *dpy, int screen ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXQueryExtensionsString( GuestToHostX11(dpy), screen));
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXQueryServerString)( Display *dpy, int screen, int name ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXQueryServerString( GuestToHostX11(dpy), screen, name));
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXGetClientString)( Display *dpy, int name ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXGetClientString( GuestToHostX11(dpy), name));
	SYNC_HOST_GUEST();
	return rv;
}


/*GLX 1.2 and later */
Display *IMPL(glXGetCurrentDisplay)( void ) {
	return PACKER(fgl_HostToGuestX11)(fexfn_pack_glXGetCurrentDisplay());
}


/*GLX 1.3 and later */
GLXFBConfig *IMPL(glXChooseFBConfig)( Display *dpy, int screen, const int *attribList, int *nitems ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXChooseFBConfig( GuestToHostX11(dpy), screen, attribList, nitems));
	SYNC_HOST_GUEST();
	return MapGLXFBConfigHostToGuest(rv, *nitems);
}

int IMPL(glXGetFBConfigAttrib)( Display *dpy, GLXFBConfig config, int attribute, int *value ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXGetFBConfigAttrib( GuestToHostX11(dpy), config, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

GLXFBConfig *IMPL(glXGetFBConfigs)( Display *dpy, int screen, int *nelements ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXGetFBConfigs( GuestToHostX11(dpy), screen, nelements));
	SYNC_HOST_GUEST();
	return MapGLXFBConfigHostToGuest(rv, *nelements);
}

XVisualInfo *IMPL(glXGetVisualFromFBConfig)( Display *dpy, GLXFBConfig config )  {
	SYNC_GUEST_HOST();
	auto rv = MapVisualInfoHostToGuest(dpy, TRACE(fexfn_pack_glXGetVisualFromFBConfig( GuestToHostX11(dpy), config)));
	SYNC_HOST_GUEST();
	return rv;
}

GLXWindow IMPL(glXCreateWindow)( Display *dpy, GLXFBConfig config, Window win, const int *attribList ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXCreateWindow( GuestToHostX11(dpy), config, win, attribList));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyWindow)( Display *dpy, GLXWindow window )  {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXDestroyWindow( GuestToHostX11(dpy), window));
	SYNC_HOST_GUEST();
}

GLXPixmap IMPL(glXCreatePixmap)( Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXCreatePixmap( GuestToHostX11(dpy), config, pixmap, attribList));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyPixmap)( Display *dpy, GLXPixmap pixmap ) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXDestroyPixmap( GuestToHostX11(dpy), pixmap));
	SYNC_HOST_GUEST();
}

GLXPbuffer IMPL(glXCreatePbuffer)( Display *dpy, GLXFBConfig config, const int *attribList ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXCreatePbuffer( GuestToHostX11(dpy), config, attribList));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyPbuffer)( Display *dpy, GLXPbuffer pbuf ) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXDestroyPbuffer( GuestToHostX11(dpy), pbuf));
	SYNC_HOST_GUEST();
}

void IMPL(glXQueryDrawable)( Display *dpy, GLXDrawable draw, int attribute, unsigned int *value ) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXQueryDrawable( GuestToHostX11(dpy), draw, attribute, value));
	SYNC_HOST_GUEST();
}

GLXContext IMPL(glXCreateNewContext)( Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXCreateNewContext( GuestToHostX11(dpy), config, renderType, shareList, direct));
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXMakeContextCurrent)( Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXMakeContextCurrent( GuestToHostX11(dpy), draw, read, ctx));
	SYNC_HOST_GUEST();
	return rv;
}

// FEX_TODO("Sync?")
GLXDrawable IMPL(glXGetCurrentReadDrawable)( void ) {
	auto rv = TRACE(fexfn_pack_glXGetCurrentReadDrawable( ));
	return rv;
}

int IMPL(glXQueryContext)( Display *dpy, GLXContext ctx, int attribute, int *value ) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXQueryContext( GuestToHostX11(dpy), ctx, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXSelectEvent)( Display *dpy, GLXDrawable drawable, unsigned long mask ) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXSelectEvent( GuestToHostX11(dpy), drawable, mask));
	SYNC_HOST_GUEST();
}

void IMPL(glXGetSelectedEvent)( Display *dpy, GLXDrawable drawable, unsigned long *mask ) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXGetSelectedEvent( GuestToHostX11(dpy), drawable, mask));
	SYNC_HOST_GUEST();
}


void IMPL(glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam) {
	STUB();
}

void IMPL(glDebugMessageCallbackAMD)(GLDEBUGPROCAMD callback, void *userParam) {
	STUB();
}

void IMPL(glDebugMessageCallbackARB)(GLDEBUGPROC callback, const void *userParam) {
	STUB();
}

/*GLX 1.3 function pointer typedefs */
// Implemented at the end of the file
voidFunc *IMPL(glXGetProcAddressARB)(const GLubyte *procname);

/*GLX 1.4 and later */
// Implemented at the end of the file
voidFunc *IMPL(glXGetProcAddress)(const GLubyte *procname);

// FEX_TODO("what about plain glXCreateContextAttribs ?")
GLXContext IMPL(glXCreateContextAttribsARB)(	Display *dpy,  GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXCreateContextAttribsARB( GuestToHostX11(dpy), config, share_context, direct, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval) {
	SYNC_GUEST_HOST();
	TRACEV(fexfn_pack_glXSwapIntervalEXT( GuestToHostX11(dpy), drawable, interval));
	SYNC_HOST_GUEST();
}

Bool IMPL(glXQueryRendererIntegerMESA) (Display *dpy, int screen, int renderer, int attribute, unsigned int *value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXQueryRendererIntegerMESA( GuestToHostX11(dpy), screen, renderer, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXQueryRendererStringMESA) (Display *dpy, int screen, int renderer, int attribute) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(fexfn_pack_glXQueryRendererStringMESA( GuestToHostX11(dpy), screen, renderer, attribute));
	SYNC_HOST_GUEST();
	return rv;
}

#if 0
// This is not direcly exported ?
void *glXAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
void glXFreeMemoryNV(GLvoid *pointer);
#endif

#if 0
// This is not direcly exported ?

Bool glXBindTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool glXReleaseTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool glXDrawableAttribARB(Display *dpy, GLXDrawable draw, const int *attribList);
#endif

#if 0
// This is not direcly exported ?
extern int glXGetFrameUsageMESA(Display *dpy, GLXDrawable drawable, float *usage);
extern int glXBeginFrameTrackingMESA(Display *dpy, GLXDrawable drawable);
extern int glXEndFrameTrackingMESA(Display *dpy, GLXDrawable drawable);
extern int glXQueryFrameTrackingMESA(Display *dpy, GLXDrawable drawable, int64_t *swapCount, int64_t *missedFrames, float *lastMissedUsage);
#endif

//?
GLXContext IMPL(glXImportContextEXT)(Display *dpy, GLXContextID contextID) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXImportContextEXT)(GuestToHostX11(dpy), contextID));
  SYNC_HOST_GUEST();
  return rv;
}

void IMPL(glXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height) {
	SYNC_GUEST_HOST();
	TRACEV(PACKER(glXCopySubBufferMESA)(GuestToHostX11(dpy), drawable, x, y, width, height));
	SYNC_HOST_GUEST();
}

Bool IMPL(glXMakeCurrentReadSGI)(Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXMakeCurrentReadSGI)(GuestToHostX11(dpy), draw, read, ctx));
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXGetSyncValuesOML)(GuestToHostX11(dpy), drawable, ust, msc, sbc));
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXGetMscRateOML)(Display* dpy, GLXDrawable drawable, int32_t* numerator, int32_t* denominator) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXGetMscRateOML)(GuestToHostX11(dpy), drawable, numerator, denominator));
  SYNC_HOST_GUEST();
  return rv;
}

int64_t IMPL(glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXSwapBuffersMscOML)(GuestToHostX11(dpy), drawable, target_msc, divisor, remainder));
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXWaitForMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t* ust, int64_t* msc, int64_t* sbc) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXWaitForMscOML)(GuestToHostX11(dpy), drawable, target_msc, divisor, remainder, ust, msc, sbc));
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXWaitForSbcOML)(Display* dpy, GLXDrawable drawable, int64_t target_sbc, int64_t* ust, int64_t* msc, int64_t* sbc) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXWaitForSbcOML)(GuestToHostX11(dpy), drawable, target_sbc, ust, msc, sbc));
  SYNC_HOST_GUEST();
  return rv;
}

int IMPL(glXQueryContextInfoEXT)(Display * dpy, GLXContext ctx, int attribute, int *value) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXQueryContextInfoEXT)(GuestToHostX11(dpy), ctx, attribute, value));
  SYNC_HOST_GUEST();
  return rv;
}

void IMPL(glXBindTexImageEXT)(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXBindTexImageEXT)(GuestToHostX11(dpy), drawable, buffer, attrib_list));
  SYNC_HOST_GUEST();
}

void IMPL(glXReleaseTexImageEXT) (Display *dpy, GLXDrawable drawable, int buffer) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXReleaseTexImageEXT)(GuestToHostX11(dpy), drawable, buffer));
  SYNC_HOST_GUEST();
}

void IMPL(glXFreeContextEXT)(Display * dpy, GLXContext ctx) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXFreeContextEXT)(GuestToHostX11(dpy), ctx));
  SYNC_HOST_GUEST();
}


GLXPbuffer IMPL(glXCreateGLXPbufferSGIX)(Display *dpy, GLXFBConfig config, unsigned int width, unsigned int height, int *attrib_list) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXCreateGLXPbufferSGIX)(GuestToHostX11(dpy), config, width, height, attrib_list));
  SYNC_HOST_GUEST();
  return rv;
}

void IMPL(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXDestroyGLXPbufferSGIX)(GuestToHostX11(dpy), pbuf));
  SYNC_HOST_GUEST();
}

void IMPL(glXQueryGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXQueryGLXPbufferSGIX)(GuestToHostX11(dpy), pbuf, attribute, value));
  SYNC_HOST_GUEST();
}

void IMPL(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXSelectEventSGIX)(GuestToHostX11(dpy), drawable, mask));
  SYNC_HOST_GUEST();
}

void IMPL(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask) {
  SYNC_GUEST_HOST();
  TRACEV(PACKER(glXGetSelectedEventSGIX)(GuestToHostX11(dpy), drawable, mask));
  SYNC_HOST_GUEST();
}

XVisualInfo *IMPL(glXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfigSGIX config) {
  SYNC_GUEST_HOST();
  auto rv = MapVisualInfoHostToGuest(dpy, TRACE(PACKER(glXGetVisualFromFBConfigSGIX)(GuestToHostX11(dpy), config)));
  SYNC_HOST_GUEST();
  return rv;
}


GLXContext IMPL(glXCreateContextWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)	{
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXCreateContextWithConfigSGIX)(GuestToHostX11(dpy), config, render_type, share_list, direct));
  SYNC_HOST_GUEST();
  return rv;
}


GLXFBConfigSGIX *IMPL(glXChooseFBConfigSGIX)(Display *dpy, int screen, int *attrib_list, int *nelements) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXChooseFBConfigSGIX)(GuestToHostX11(dpy), screen, attrib_list, nelements));
  SYNC_HOST_GUEST();
  return MapGLXFBConfigSGIXHostToGuest(rv, *nelements);
}


GLXFBConfigSGIX IMPL(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis) {
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXGetFBConfigFromVisualSGIX)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, vis)));
  SYNC_HOST_GUEST();
  return rv;
}


GLXPixmap IMPL(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)	{
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXCreateGLXPixmapWithConfigSGIX)(GuestToHostX11(dpy), config, pixmap));
  SYNC_HOST_GUEST();
  return rv;
}


int IMPL(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value)	{
  SYNC_GUEST_HOST();
  auto rv = TRACE(PACKER(glXGetFBConfigAttribSGIX)(GuestToHostX11(dpy), config, attribute, value));
  SYNC_HOST_GUEST();
  return rv;
}

/*

	glx also exports __glXGLLoadGLXFunction and 
	__GLXGL_CORE_FUNCTIONS (data symbol)
*/

// EGL

#undef SYNC_GUEST_HOST
#undef SYNC_HOST_GUEST

#define SYNC_GUEST_HOST() do { \
		auto dpy = PACKER(fgl_HostToXGuestEGL)(display); \
		if (dpy) XSync(dpy, False); \
		PACKER(fgl_FlushFromHostEGL)(display);\
	} while(0)


#define SYNC_HOST_GUEST() do { \
		auto dpy = PACKER(fgl_FlushFromHostEGL)(display); \
		if (dpy) XSync(dpy, False); \
	} while(0)

static int DisplayCloseCallbackEGL(Display *dpy, XPointer data)
{
	PACKER(fgl_RemoveXGuestEGL)(dpy);
	return 0;
}

static Display *XGuestToXHostEGL(Display *dpy) {
	auto rv = PACKER(fgl_XGuestToXHostEGL)(dpy);

	if (!rv) {
		XmuAddCloseDisplayHook(dpy, &DisplayCloseCallbackEGL, nullptr);
		rv = PACKER(fgl_AddXGuestEGL)(dpy, XDisplayString(dpy));

		if (!rv) {
			printf("XGuestToXHostEGL: fgl_AddXGuestEGL failed\n");
		}
	}
	return rv;
}

EGLBoolean IMPL(eglBindAPI)(EGLenum api) {
	auto rv = TRACE(PACKER(eglBindAPI)(api));
	return rv;
}

EGLBoolean IMPL(eglChooseConfig)(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglChooseConfig)(display, attrib_list, configs, config_size, num_config));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglDestroyContext)(EGLDisplay display, EGLContext context) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglDestroyContext)(display, context));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglDestroySurface)(EGLDisplay display, EGLSurface surface) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglDestroySurface)(display, surface));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglInitialize)(EGLDisplay display, EGLint *major, EGLint *minor) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglInitialize)(display, major, minor));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglMakeCurrent)(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglMakeCurrent)(display, draw, read, context));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglQuerySurface)(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglQuerySurface)(display, surface, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglSurfaceAttrib)(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglSurfaceAttrib)(display, surface, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglSwapBuffers)(EGLDisplay display, EGLSurface surface) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglSwapBuffers)(display, surface));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglTerminate)(EGLDisplay display) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglTerminate)(display));
	// no sync here
	return rv;
}

EGLContext IMPL(eglCreateContext)(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreateContext)(display, config, share_context, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLSurface IMPL(eglCreateWindowSurface)(EGLDisplay display, EGLConfig config, NativeWindowType native_window, EGLint const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreateWindowSurface)(display, config, native_window, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLDisplay IMPL(eglGetDisplay)(NativeDisplayType native_display) {
	EGLDisplay display;
	
	if (native_display != EGL_DEFAULT_DISPLAY) {
		auto dpy = (Display*)native_display;
		XFlush(dpy);
		display = TRACE(PACKER(eglGetDisplay)(XGuestToXHostEGL(dpy)));
		SYNC_HOST_GUEST();
	} else {
		display = TRACE(PACKER(eglGetDisplay)(native_display));
	}
	
	return display;
}

EGLDisplay IMPL(eglGetPlatformDisplay)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list) {
	printf("eglGetPlatformDisplay: not implemented\n");
	return EGL_NO_DISPLAY;
}

EGLBoolean IMPL(eglBindTexImage)(EGLDisplay display, EGLSurface surface, EGLint buffer) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglBindTexImage)(display, surface, buffer));
	SYNC_HOST_GUEST();
	return rv;
}


EGLint IMPL(eglClientWaitSync)(EGLDisplay display, EGLSync sync, EGLint flags, EGLTime timeout) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglClientWaitSync)(display, sync, flags, timeout));
	SYNC_HOST_GUEST();
	return rv;
}


EGLBoolean IMPL(eglCopyBuffers)(EGLDisplay display, EGLSurface surface, NativePixmapType native_pixmap) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCopyBuffers)(display, surface, native_pixmap));
	SYNC_HOST_GUEST();
	return rv;
}

EGLImage IMPL(eglCreateImage)(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreateImage)(display, context, target, buffer, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}


EGLSurface IMPL(eglCreatePbufferFromClientBuffer)(EGLDisplay display, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, EGLint const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreatePbufferFromClientBuffer)(display, buftype, buffer, config, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}


EGLSurface IMPL(eglCreatePbufferSurface)(EGLDisplay display, EGLConfig config, EGLint const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreatePbufferSurface)(display, config, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLSurface IMPL(eglCreatePixmapSurface)(EGLDisplay display, EGLConfig config, NativePixmapType native_pixmap, EGLint const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreatePixmapSurface)(display, config, native_pixmap, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}


EGLSurface IMPL(eglCreatePlatformPixmapSurface)(EGLDisplay display, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreatePlatformPixmapSurface)(display, config, native_pixmap, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLSurface IMPL(eglCreatePlatformWindowSurface)(EGLDisplay display, EGLConfig config, void *native_window, EGLAttrib const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreatePlatformWindowSurface)(display, config, native_window, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLSync IMPL(eglCreateSync)(EGLDisplay display, EGLenum type, EGLAttrib const *attrib_list) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglCreateSync)(display, type, attrib_list));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglDestroyImage)(EGLDisplay display, EGLImage image) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglDestroyImage)(display, image));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglDestroySync)(EGLDisplay display, EGLSync sync) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglDestroySync)(display, sync));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglGetConfigAttrib)(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglGetConfigAttrib)(display, config, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglGetConfigs)(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglGetConfigs)(display, configs, config_size, num_config));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglGetSyncAttrib)(EGLDisplay display, EGLSync sync, EGLint attribute, EGLAttrib *value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglGetSyncAttrib)(display, sync, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglQueryContext)(EGLDisplay display, EGLContext context, EGLint attribute, EGLint *value) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglQueryContext)(display, context, attribute, value));
	SYNC_HOST_GUEST();
	return rv;
}

char const *IMPL(eglQueryString)(EGLDisplay display, EGLint name) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglQueryString)(display, name));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglReleaseTexImage)(EGLDisplay display, EGLSurface surface, EGLint buffer) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglReleaseTexImage)(display, surface, buffer));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglSwapInterval)(EGLDisplay display, EGLint interval) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglSwapInterval)(display, interval));
	SYNC_HOST_GUEST();
	return rv;
}

EGLBoolean IMPL(eglWaitSync)(EGLDisplay display, EGLSync sync, EGLint flags) {
	SYNC_GUEST_HOST();
	auto rv = TRACE(PACKER(eglWaitSync)(display, sync, flags));
	SYNC_HOST_GUEST();
	return rv;
}

// Implemented at the end of the file
voidFunc *IMPL(eglGetProcAddress)(const char *procname);
}

#include "function_packs_public.inl"

// These are direct exports
extern "C" {
voidFunc *IMPL(glXGetProcAddress)(const GLubyte *procname) {

	for (int i = 0; gl_symtable[i].name; i++) {
		if (strcmp(gl_symtable[i].name, (const char*)procname) == 0) {
			// for debugging
			dbgf("glXGetProcAddress: looked up %s %s %p %p\n", procname, gl_symtable[i].name, gl_symtable[i].fn, &glXGetProcAddress);
			return gl_symtable[i].fn;
		}
	}

	for (int i = 0; glx_symtable[i].name; i++) {
		if (strcmp(glx_symtable[i].name, (const char*)procname) == 0) {
			// for debugging
			dbgf("glXGetProcAddress: looked up %s %s %p %p\n", procname, glx_symtable[i].name, glx_symtable[i].fn, &glXGetProcAddress);
			return glx_symtable[i].fn;
		}
	}

	errf("glXGetProcAddress: not found %s\n", procname);
	return nullptr;
}

voidFunc *IMPL(glXGetProcAddressARB)(const GLubyte *procname) {
	return glXGetProcAddress(procname);
}

voidFunc *IMPL(eglGetProcAddress)(const char *procname) {
	for (int i = 0; gl_symtable[i].name; i++) {
		if (strcmp(gl_symtable[i].name, (const char*)procname) == 0) {
			dbgf("eglGetProcAddress: looked up %s %s %p %p\n", procname, gl_symtable[i].name, gl_symtable[i].fn, &eglGetProcAddress);
			return gl_symtable[i].fn;
		}
	}

	for (int i = 0; egl_symtable[i].name; i++) {
		if (strcmp(egl_symtable[i].name, (const char*)procname) == 0) {
			dbgf("eglGetProcAddress: looked up %s %s %p %p\n", procname, egl_symtable[i].name, egl_symtable[i].fn, &eglGetProcAddress);
			return egl_symtable[i].fn;
		}
	}

	errf("eglGetProcAddress: not found %s\n", procname);
	return nullptr;
}
}

LOAD_LIB(libFGL)