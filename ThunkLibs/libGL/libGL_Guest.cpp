/*
$info$
tags: thunklibs|GL
desc: Handles glXGetProcAddress
$end_info$
*/

#define XLIB_ILLEGAL_ACCESS

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdio.h>
#include <cstring>
#include <cassert>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

// Thank you x11 devs
#define Xmalloc malloc
#include <cstdlib>
//#include <X11/Xlibint.h>

typedef void voidFunc();

#define STUB() do { errf("GLX: Stub\n"); assert(false && __FUNCTION__); } while (0)


#if defined(TRACE_GLX)
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


#include "private_api.h"

#include <shared_mutex>
#include <map>

#define SYNC_GUEST_HOST() do { XFlush(dpy); fgl_FlushFromGuest(dpy, dpy->display_name); } while(0)
#define SYNC_HOST_GUEST() do { fgl_FlushFromGuest(dpy, dpy->display_name); XFlush(dpy); } while(0)

static XVisualInfo *MapVisualInfoHostToGuest(Display *dpy, XVisualInfo *HostVis) {
	if (!HostVis) {
		dbgf("MapVisualInfoHostToGuest: Can't map null HostVis\n");
		return nullptr;
	}

	XVisualInfo v;
	
	// FEX_TODO("HostVis might not be same as guest XVisualInfo here")
	v.screen = HostVis->screen;
	v.visualid = HostVis->visualid;

	fgl_XFree(HostVis);

	int c;
	auto vguest = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask, &v, &c);

	if (c >= 1 && vguest != nullptr) {
		return vguest;
	} else {
		dbgf("MapVisualInfoHostToGuest: Guest XGetVisualInfo returned null\n");
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

	auto rv = (GLXFBConfig *)Xmalloc(sizeof(GLXFBConfig) * count);

	if (!rv) {
		dbgf("MapGLXFBConfigHostToGuest: Xmalloc failed\n");
		return nullptr;
	}

	for (int i = 0; i < count; i++) {
		rv[i] = Host[i];
	}
	
	fgl_XFree(Host);

	return rv;
}

extern "C" {
	voidFunc *glXGetProcAddress(const GLubyte *procname) {

        for (int i = 0; internal_symtable[i].name; i++) {
            if (strcmp(internal_symtable[i].name, (const char*)procname) == 0) {
				// for debugging
                //dbgf("glXGetProcAddress: looked up %s %s %p %p\n", procname, internal_symtable[i].name, internal_symtable[i].fn, &glXGetProcAddress);
                return internal_symtable[i].fn;
			}
		}

/*
		for (int i = 0; _symtable[i].name; i++) {
            if (strcmp(_symtable[i].name, (const char*)procname) == 0) {
				// for debugging
                //dbgf("glXGetProcAddress: looked up %s %s %p %p\n", procname, internal_symtable[i].name, internal_symtable[i].fn, &glXGetProcAddress);
                return _symtable[i].fn;
			}
		}*/

		errf("glXGetProcAddress: not found %s\n", procname);
		return nullptr;
	}

	voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
		return glXGetProcAddress(procname);
	}


	XVisualInfo *glXChooseVisual( Display *dpy, int screen, int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = MapVisualInfoHostToGuest(dpy, TRACE(fexfn_pack_glXChooseVisual( fgl_GuestToHost(dpy, dpy->display_name), screen, attribList)));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateContext( fgl_GuestToHost(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, vis), shareList, direct));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyContext( Display *dpy, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyContext( fgl_GuestToHost(dpy, dpy->display_name), ctx));
		SYNC_GUEST_HOST();
	}

	Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXMakeCurrent( fgl_GuestToHost(dpy, dpy->display_name), drawable, ctx));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXCopyContext( fgl_GuestToHost(dpy, dpy->display_name), src, dst, mask));
		SYNC_HOST_GUEST();
	}

	void glXSwapBuffers( Display *dpy, GLXDrawable drawable ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSwapBuffers( fgl_GuestToHost(dpy, dpy->display_name), drawable));
		SYNC_HOST_GUEST();
	}

	GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual, Pixmap pixmap ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateGLXPixmap( fgl_GuestToHost(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, visual), pixmap));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyGLXPixmap( fgl_GuestToHost(dpy, dpy->display_name), pixmap));
		SYNC_HOST_GUEST();
	}

	Bool glXQueryExtension( Display *dpy, int *errorb, int *event ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryExtension( fgl_GuestToHost(dpy, dpy->display_name), errorb, event));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXQueryVersion( Display *dpy, int *maj, int *min ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryVersion( fgl_GuestToHost(dpy, dpy->display_name), maj, min));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXIsDirect( Display *dpy, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXIsDirect( fgl_GuestToHost(dpy, dpy->display_name), ctx));
		SYNC_HOST_GUEST();
		return rv;
	}

	int glXGetConfig( Display *dpy, XVisualInfo *visual, int attrib, int *value ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetConfig( fgl_GuestToHost(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, visual), attrib, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	// FEX_TODO("Sync?")
	GLXContext glXGetCurrentContext( void ) {
		auto rv = TRACE(fexfn_pack_glXGetCurrentContext());
		return rv;
	}

	// FEX_TODO("Sync?")
	GLXDrawable glXGetCurrentDrawable( void ) {
		auto rv = TRACE(fexfn_pack_glXGetCurrentDrawable());
		return rv;
	}

	// FEX_TODO("Sync?")
	void glXWaitGL( void ) {
		TRACEV(fexfn_pack_glXWaitGL());
	}

	// FEX_TODO("Sync?")
	void glXWaitX( void ) {
		TRACEV(fexfn_pack_glXWaitX());
	}

	/*
	void glXUseXFont( Font font, int first, int count, int list ) {
		STUB();
	}
	*/



	/* GLX 1.1 and later */
	const char *glXQueryExtensionsString( Display *dpy, int screen ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryExtensionsString( fgl_GuestToHost(dpy, dpy->display_name), screen));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXQueryServerString( Display *dpy, int screen, int name ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryServerString( fgl_GuestToHost(dpy, dpy->display_name), screen, name));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXGetClientString( Display *dpy, int name ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetClientString( fgl_GuestToHost(dpy, dpy->display_name), name));
		SYNC_HOST_GUEST();
		return rv;
	}


	/* GLX 1.2 and later */
	Display *glXGetCurrentDisplay( void ) {
		return fgl_HostToGuest(fexfn_pack_glXGetCurrentDisplay());
	}


	/* GLX 1.3 and later */
	GLXFBConfig *glXChooseFBConfig( Display *dpy, int screen, const int *attribList, int *nitems ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXChooseFBConfig( fgl_GuestToHost(dpy, dpy->display_name), screen, attribList, nitems));
		SYNC_HOST_GUEST();
		return MapGLXFBConfigHostToGuest(rv, *nitems);
	}

	int glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config, int attribute, int *value ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetFBConfigAttrib( fgl_GuestToHost(dpy, dpy->display_name), config, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXFBConfig *glXGetFBConfigs( Display *dpy, int screen, int *nelements ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetFBConfigs( fgl_GuestToHost(dpy, dpy->display_name), screen, nelements));
		SYNC_HOST_GUEST();
		return MapGLXFBConfigHostToGuest(rv, *nelements);
	}

	XVisualInfo *glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )  {
		SYNC_GUEST_HOST();
		auto rv = MapVisualInfoHostToGuest(dpy, TRACE(fexfn_pack_glXGetVisualFromFBConfig( fgl_GuestToHost(dpy, dpy->display_name), config)));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXWindow glXCreateWindow( Display *dpy, GLXFBConfig config, Window win, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateWindow( fgl_GuestToHost(dpy, dpy->display_name), config, win, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyWindow( Display *dpy, GLXWindow window )  {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyWindow( fgl_GuestToHost(dpy, dpy->display_name), window));
		SYNC_HOST_GUEST();
	}

	GLXPixmap glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreatePixmap( fgl_GuestToHost(dpy, dpy->display_name), config, pixmap, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyPixmap( Display *dpy, GLXPixmap pixmap ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyPixmap( fgl_GuestToHost(dpy, dpy->display_name), pixmap));
		SYNC_HOST_GUEST();
	}

	GLXPbuffer glXCreatePbuffer( Display *dpy, GLXFBConfig config, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreatePbuffer( fgl_GuestToHost(dpy, dpy->display_name), config, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyPbuffer( fgl_GuestToHost(dpy, dpy->display_name), pbuf));
		SYNC_HOST_GUEST();
	}

	void glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute, unsigned int *value ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXQueryDrawable( fgl_GuestToHost(dpy, dpy->display_name), draw, attribute, value));
		SYNC_HOST_GUEST();
	}

	GLXContext glXCreateNewContext( Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateNewContext( fgl_GuestToHost(dpy, dpy->display_name), config, renderType, shareList, direct));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXMakeContextCurrent( Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXMakeContextCurrent( fgl_GuestToHost(dpy, dpy->display_name), draw, read, ctx));
		SYNC_HOST_GUEST();
		return rv;
	}

	// FEX_TODO("Sync?")
	GLXDrawable glXGetCurrentReadDrawable( void ) {
		auto rv = TRACE(fexfn_pack_glXGetCurrentReadDrawable( ));
		return rv;
	}

	int glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryContext( fgl_GuestToHost(dpy, dpy->display_name), ctx, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSelectEvent( fgl_GuestToHost(dpy, dpy->display_name), drawable, mask));
		SYNC_HOST_GUEST();
	}

	void glXGetSelectedEvent( Display *dpy, GLXDrawable drawable, unsigned long *mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXGetSelectedEvent( fgl_GuestToHost(dpy, dpy->display_name), drawable, mask));
		SYNC_HOST_GUEST();
	}


	void glDebugMessageCallback(GLDEBUGPROC callback, const void * userParam) {
		STUB();
	}

	void glDebugMessageCallbackAMD(GLDEBUGPROCAMD callback, void * userParam) {
		STUB();
	}

	void glDebugMessageCallbackARB(GLDEBUGPROC callback, const void * userParam) {
		STUB();
	}

	/* GLX 1.3 function pointer typedefs */
	/*
	//implemented above
	__GLXextFuncPtr glXGetProcAddressARB (const GLubyte *) {
		STUB();
	}

	*/

	/* GLX 1.4 and later */
	/*
	//implemented above
	void (*glXGetProcAddress(const GLubyte *procname))( void ) {
		STUB();
	}
	*/

	// FEX_TODO("what about plain glXCreateContextAttribs ?")
	GLXContext glXCreateContextAttribsARB(	Display * dpy,  GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateContextAttribsARB( fgl_GuestToHost(dpy, dpy->display_name), config, share_context, direct, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXSwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSwapIntervalEXT( fgl_GuestToHost(dpy, dpy->display_name), drawable, interval));
		SYNC_HOST_GUEST();
	}

	Bool glXQueryRendererIntegerMESA (Display *dpy, int screen, int renderer, int attribute, unsigned int *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryRendererIntegerMESA( fgl_GuestToHost(dpy, dpy->display_name), screen, renderer, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXQueryRendererStringMESA (Display *dpy, int screen, int renderer, int attribute) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryRendererStringMESA( fgl_GuestToHost(dpy, dpy->display_name), screen, renderer, attribute));
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

	/*

		glx also exports __glXGLLoadGLXFunction and 
		__GLXGL_CORE_FUNCTIONS (data symbol)
	*/
}


LOAD_LIB(libGL)