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

#include <EGL/egl.h>

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

#include "libGL_private.h"

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


#include <shared_mutex>
#include <map>

#define SYNC_GUEST_HOST() do { XFlush(dpy); fgl_FlushFromGuestX11(dpy, dpy->display_name); } while(0)
#define SYNC_HOST_GUEST() do { fgl_FlushFromGuestX11(dpy, dpy->display_name); XFlush(dpy); } while(0)

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

	auto rv = (GLXFBConfig *)Xmalloc(sizeof(GLXFBConfig) *count);

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
		auto rv = MapVisualInfoHostToGuest(dpy, TRACE(fexfn_pack_glXChooseVisual( fgl_GuestToHostX11(dpy, dpy->display_name), screen, attribList)));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateContext( fgl_GuestToHostX11(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, vis), shareList, direct));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyContext( Display *dpy, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyContext( fgl_GuestToHostX11(dpy, dpy->display_name), ctx));
		SYNC_GUEST_HOST();
	}

	Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXMakeCurrent( fgl_GuestToHostX11(dpy, dpy->display_name), drawable, ctx));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXCopyContext( fgl_GuestToHostX11(dpy, dpy->display_name), src, dst, mask));
		SYNC_HOST_GUEST();
	}

	void glXSwapBuffers( Display *dpy, GLXDrawable drawable ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSwapBuffers( fgl_GuestToHostX11(dpy, dpy->display_name), drawable));
		SYNC_HOST_GUEST();
	}

	GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual, Pixmap pixmap ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateGLXPixmap( fgl_GuestToHostX11(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, visual), pixmap));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyGLXPixmap( fgl_GuestToHostX11(dpy, dpy->display_name), pixmap));
		SYNC_HOST_GUEST();
	}

	Bool glXQueryExtension( Display *dpy, int *errorb, int *event ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryExtension( fgl_GuestToHostX11(dpy, dpy->display_name), errorb, event));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXQueryVersion( Display *dpy, int *maj, int *min ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryVersion( fgl_GuestToHostX11(dpy, dpy->display_name), maj, min));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXIsDirect( Display *dpy, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXIsDirect( fgl_GuestToHostX11(dpy, dpy->display_name), ctx));
		SYNC_HOST_GUEST();
		return rv;
	}

	int glXGetConfig( Display *dpy, XVisualInfo *visual, int attrib, int *value ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetConfig( fgl_GuestToHostX11(dpy, dpy->display_name), MapVisualInfoGuestToHost(dpy, visual), attrib, value));
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



	/*GLX 1.1 and later */
	const char *glXQueryExtensionsString( Display *dpy, int screen ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryExtensionsString( fgl_GuestToHostX11(dpy, dpy->display_name), screen));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXQueryServerString( Display *dpy, int screen, int name ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryServerString( fgl_GuestToHostX11(dpy, dpy->display_name), screen, name));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXGetClientString( Display *dpy, int name ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetClientString( fgl_GuestToHostX11(dpy, dpy->display_name), name));
		SYNC_HOST_GUEST();
		return rv;
	}


	/*GLX 1.2 and later */
	Display *glXGetCurrentDisplay( void ) {
		return fgl_HostToGuestX11(fexfn_pack_glXGetCurrentDisplay());
	}


	/*GLX 1.3 and later */
	GLXFBConfig *glXChooseFBConfig( Display *dpy, int screen, const int *attribList, int *nitems ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXChooseFBConfig( fgl_GuestToHostX11(dpy, dpy->display_name), screen, attribList, nitems));
		SYNC_HOST_GUEST();
		return MapGLXFBConfigHostToGuest(rv, *nitems);
	}

	int glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config, int attribute, int *value ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetFBConfigAttrib( fgl_GuestToHostX11(dpy, dpy->display_name), config, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXFBConfig *glXGetFBConfigs( Display *dpy, int screen, int *nelements ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXGetFBConfigs( fgl_GuestToHostX11(dpy, dpy->display_name), screen, nelements));
		SYNC_HOST_GUEST();
		return MapGLXFBConfigHostToGuest(rv, *nelements);
	}

	XVisualInfo *glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )  {
		SYNC_GUEST_HOST();
		auto rv = MapVisualInfoHostToGuest(dpy, TRACE(fexfn_pack_glXGetVisualFromFBConfig( fgl_GuestToHostX11(dpy, dpy->display_name), config)));
		SYNC_HOST_GUEST();
		return rv;
	}

	GLXWindow glXCreateWindow( Display *dpy, GLXFBConfig config, Window win, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateWindow( fgl_GuestToHostX11(dpy, dpy->display_name), config, win, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyWindow( Display *dpy, GLXWindow window )  {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyWindow( fgl_GuestToHostX11(dpy, dpy->display_name), window));
		SYNC_HOST_GUEST();
	}

	GLXPixmap glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreatePixmap( fgl_GuestToHostX11(dpy, dpy->display_name), config, pixmap, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyPixmap( Display *dpy, GLXPixmap pixmap ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyPixmap( fgl_GuestToHostX11(dpy, dpy->display_name), pixmap));
		SYNC_HOST_GUEST();
	}

	GLXPbuffer glXCreatePbuffer( Display *dpy, GLXFBConfig config, const int *attribList ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreatePbuffer( fgl_GuestToHostX11(dpy, dpy->display_name), config, attribList));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXDestroyPbuffer( fgl_GuestToHostX11(dpy, dpy->display_name), pbuf));
		SYNC_HOST_GUEST();
	}

	void glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute, unsigned int *value ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXQueryDrawable( fgl_GuestToHostX11(dpy, dpy->display_name), draw, attribute, value));
		SYNC_HOST_GUEST();
	}

	GLXContext glXCreateNewContext( Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateNewContext( fgl_GuestToHostX11(dpy, dpy->display_name), config, renderType, shareList, direct));
		SYNC_HOST_GUEST();
		return rv;
	}

	Bool glXMakeContextCurrent( Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx ) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXMakeContextCurrent( fgl_GuestToHostX11(dpy, dpy->display_name), draw, read, ctx));
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
		auto rv = TRACE(fexfn_pack_glXQueryContext( fgl_GuestToHostX11(dpy, dpy->display_name), ctx, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSelectEvent( fgl_GuestToHostX11(dpy, dpy->display_name), drawable, mask));
		SYNC_HOST_GUEST();
	}

	void glXGetSelectedEvent( Display *dpy, GLXDrawable drawable, unsigned long *mask ) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXGetSelectedEvent( fgl_GuestToHostX11(dpy, dpy->display_name), drawable, mask));
		SYNC_HOST_GUEST();
	}


	void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) {
		STUB();
	}

	void glDebugMessageCallbackAMD(GLDEBUGPROCAMD callback, void *userParam) {
		STUB();
	}

	void glDebugMessageCallbackARB(GLDEBUGPROC callback, const void *userParam) {
		STUB();
	}

	/*GLX 1.3 function pointer typedefs */
	/*
	//implemented above
	__GLXextFuncPtr glXGetProcAddressARB (const GLubyte *) {
		STUB();
	}

	*/

	/*GLX 1.4 and later */
	/*
	//implemented above
	void (*glXGetProcAddress(const GLubyte *procname))( void ) {
		STUB();
	}
	*/

	// FEX_TODO("what about plain glXCreateContextAttribs ?")
	GLXContext glXCreateContextAttribsARB(	Display *dpy,  GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXCreateContextAttribsARB( fgl_GuestToHostX11(dpy, dpy->display_name), config, share_context, direct, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	void glXSwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval) {
		SYNC_GUEST_HOST();
		TRACEV(fexfn_pack_glXSwapIntervalEXT( fgl_GuestToHostX11(dpy, dpy->display_name), drawable, interval));
		SYNC_HOST_GUEST();
	}

	Bool glXQueryRendererIntegerMESA (Display *dpy, int screen, int renderer, int attribute, unsigned int *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryRendererIntegerMESA( fgl_GuestToHostX11(dpy, dpy->display_name), screen, renderer, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	const char *glXQueryRendererStringMESA (Display *dpy, int screen, int renderer, int attribute) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(fexfn_pack_glXQueryRendererStringMESA( fgl_GuestToHostX11(dpy, dpy->display_name), screen, renderer, attribute));
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

	// EGL
	voidFunc *eglGetProcAddress(const char *procname) {
		return glXGetProcAddress((const GLubyte*)procname);
	}

	#define PACKER(name) fexfn_pack_##name

	#undef SYNC_GUEST_HOST
	#undef SYNC_HOST_GUEST
	
	#define SYNC_GUEST_HOST() do { \
			auto dpy = fgl_HostToXGuestEGL(display); \
			if (dpy) XFlush(dpy); \
			fgl_FlushFromHostEGL(display);\
		} while(0)


	#define SYNC_HOST_GUEST() do { \
			auto dpy = fgl_FlushFromHostEGL(display); \
			if (dpy) XFlush(dpy); \
		} while(0)

	#if EGL_CUSTOM_WIP
	EGLBoolean eglBindAPI(EGLenum api)

	EGLint eglGetError(void);

	EGLContext eglGetCurrentContext(void);
	
	EGLDisplay eglGetCurrentDisplay(void);
	
	EGLSurface eglGetCurrentSurface(EGLint readdraw);
	#endif

	EGLBoolean eglBindAPI(EGLenum api) {
		auto rv = TRACE(PACKER(eglBindAPI)(api));
		return rv;
	}

	EGLBoolean eglChooseConfig(	EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglChooseConfig)(display, attrib_list, configs, config_size, num_config));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglDestroyContext)(display, context));
		SYNC_HOST_GUEST();
		return rv;
	}
	
	EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglDestroySurface)(display, surface));
		SYNC_HOST_GUEST();
		return rv;
	}
	
	EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglInitialize)(display, major, minor));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglMakeCurrent)(display, draw, read, context));
		SYNC_HOST_GUEST();
		return rv;
	}
	
	EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglQuerySurface)(display, surface, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglSurfaceAttrib(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglSurfaceAttrib)(display, surface, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglSwapBuffers)(display, surface));
		SYNC_HOST_GUEST();
		return rv;
	}
	
	EGLBoolean eglTerminate(EGLDisplay display) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglTerminate)(display));
		// no sync here
		return rv;
	}
	
	EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreateContext)(display, config, share_context, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}
	
	EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, NativeWindowType native_window, EGLint const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreateWindowSurface)(display, config, native_window, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLDisplay eglGetDisplay(NativeDisplayType native_display) {
		EGLDisplay display;
		
		if (native_display != EGL_DEFAULT_DISPLAY) {
			auto dpy = (Display*)native_display;
			XFlush(dpy);
			display = TRACE(PACKER(eglGetDisplay)(fgl_XGuestToXHostEGL(dpy, dpy->display_name)));
			SYNC_HOST_GUEST();
		} else {
			display = TRACE(PACKER(eglGetDisplay)(native_display));
		}
		
		return display;
	}

	EGLDisplay eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list) {
		printf("eglGetPlatformDisplay: not implemented\n");
		return EGL_NO_DISPLAY;
	}

	EGLBoolean eglBindTexImage(EGLDisplay display, EGLSurface surface, EGLint buffer) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglBindTexImage)(display, surface, buffer));
		SYNC_HOST_GUEST();
		return rv;
	}


	EGLint eglClientWaitSync(EGLDisplay display, EGLSync sync, EGLint flags, EGLTime timeout) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglClientWaitSync)(display, sync, flags, timeout));
		SYNC_HOST_GUEST();
		return rv;
	}


	EGLBoolean eglCopyBuffers(EGLDisplay display, EGLSurface surface, NativePixmapType native_pixmap) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCopyBuffers)(display, surface, native_pixmap));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLImage eglCreateImage(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreateImage)(display, context, target, buffer, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}


	EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay display, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, EGLint const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreatePbufferFromClientBuffer)(display, buftype, buffer, config, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}


	EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config, EGLint const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreatePbufferSurface)(display, config, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLSurface eglCreatePixmapSurface(EGLDisplay display, EGLConfig config, NativePixmapType native_pixmap, EGLint const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreatePixmapSurface)(display, config, native_pixmap, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}


	EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay display, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreatePlatformPixmapSurface)(display, config, native_pixmap, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLSurface eglCreatePlatformWindowSurface(EGLDisplay display, EGLConfig config, void *native_window, EGLAttrib const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreatePlatformWindowSurface)(display, config, native_window, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLSync eglCreateSync(EGLDisplay display, EGLenum type, EGLAttrib const *attrib_list) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglCreateSync)(display, type, attrib_list));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglDestroyImage(EGLDisplay display, EGLImage image) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglDestroyImage)(display, image));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglDestroySync(EGLDisplay display, EGLSync sync) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglDestroySync)(display, sync));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglGetConfigAttrib)(display, config, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglGetConfigs)(display, configs, config_size, num_config));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglGetSyncAttrib(EGLDisplay display, EGLSync sync, EGLint attribute, EGLAttrib *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglGetSyncAttrib)(display, sync, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglQueryContext(EGLDisplay display, EGLContext context, EGLint attribute, EGLint *value) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglQueryContext)(display, context, attribute, value));
		SYNC_HOST_GUEST();
		return rv;
	}

	char const *eglQueryString(EGLDisplay display, EGLint name) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglQueryString)(display, name));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglReleaseTexImage(EGLDisplay display, EGLSurface surface, EGLint buffer) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglReleaseTexImage)(display, surface, buffer));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglSwapInterval)(display, interval));
		SYNC_HOST_GUEST();
		return rv;
	}

	EGLBoolean eglWaitSync(EGLDisplay display, EGLSync sync, EGLint flags) {
		SYNC_GUEST_HOST();
		auto rv = TRACE(PACKER(eglWaitSync)(display, sync, flags));
		SYNC_HOST_GUEST();
		return rv;
	}

}


LOAD_LIB(libGL)