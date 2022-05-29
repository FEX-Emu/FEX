#include <common/GeneratorInterface.h>

#include "glincludes.inl"


#define IMPL(x) x
#include "libFGL_private.h"


namespace fgl_internal {
template<auto>
struct fex_gen_config {};
// : fexgen::generate_guest_symtable { };

// internal
template<> struct fex_gen_config<fgl_AddGuestX11>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_RemoveGuestX11>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_HostToGuestX11>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_GuestToHostX11>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_XFree>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_FlushFromGuestX11>: fexgen::custom_host_impl, fexgen::no_guest_export {};

template<> struct fex_gen_config<fgl_AddXGuestEGL>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_RemoveXGuestEGL>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_HostToXGuestEGL>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_XGuestToXHostEGL>: fexgen::custom_host_impl, fexgen::no_guest_export {};
template<> struct fex_gen_config<fgl_FlushFromHostEGL>: fexgen::custom_host_impl, fexgen::no_guest_export {};
}

namespace glx {
template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable {
    const char* load_host_endpoint_via = "symbolFromGlXGetProcAddr";
};

// GLX
template<> struct fex_gen_config<glXSelectEventSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetVisualFromFBConfigSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateContextWithConfigSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXChooseFBConfigSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetFBConfigFromVisualSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateGLXPbufferSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateGLXPixmapWithConfigSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetFBConfigAttribSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyGLXPbufferSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetSelectedEventSGIX>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryGLXPbufferSGIX>: fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<glXGetContextIDEXT> {};
template<> struct fex_gen_config<glXImportContextEXT>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetCurrentReadDrawableSGI> {};
template<> struct fex_gen_config<glXSwapBuffersMscOML>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetMscRateOML>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetSwapIntervalMESA> {};
template<> struct fex_gen_config<glXGetSyncValuesOML>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetVideoSyncSGI> {};
template<> struct fex_gen_config<glXMakeCurrentReadSGI>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryContextInfoEXT>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXSwapIntervalMESA> {};
template<> struct fex_gen_config<glXSwapIntervalSGI> {};
template<> struct fex_gen_config<glXWaitForMscOML>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXWaitForSbcOML>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXWaitVideoSyncSGI> {};
template<> struct fex_gen_config<glXBindTexImageEXT>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCopySubBufferMESA>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXFreeContextEXT>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXReleaseTexImageEXT>: fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<glXGetClientString>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryExtensionsString>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryServerString>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetCurrentDisplay>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateNewContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetCurrentContext> {};
template<> struct fex_gen_config<glXGetCurrentDrawable> {};
template<> struct fex_gen_config<glXGetCurrentReadDrawable>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXChooseFBConfig>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetFBConfigs>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreatePbuffer>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateGLXPixmap>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreatePixmap>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateWindow>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetConfig>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetFBConfigAttrib>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXIsDirect>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXMakeContextCurrent>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXMakeCurrent>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryExtension>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryVersion>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXAllocateMemoryNV> {};
template<> struct fex_gen_config<glXCopyContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyGLXPixmap>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyPbuffer>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyPixmap>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXDestroyWindow>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXFreeMemoryNV> {};
template<> struct fex_gen_config<glXGetSelectedEvent>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryDrawable>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXSelectEvent>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXSwapBuffers>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXUseXFont> {}; // woo, we can pass through this one ~)
template<> struct fex_gen_config<glXWaitGL> {};
template<> struct fex_gen_config<glXWaitX> {};
template<> struct fex_gen_config<glXChooseVisual>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXGetVisualFromFBConfig>: fexgen::custom_guest_entrypoint {};

//template<> struct fex_gen_config<glXCreateContextAttribs>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXCreateContextAttribsARB>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXSwapIntervalEXT>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryCurrentRendererIntegerMESA> {}; // passthrough
template<> struct fex_gen_config<glXQueryCurrentRendererStringMESA> {}; // passthrough
template<> struct fex_gen_config<glXQueryRendererIntegerMESA>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<glXQueryRendererStringMESA>: fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<glXGetProcAddress>: fexgen::custom_guest_entrypoint, fexgen::no_host_impl, fexgen::returns_guest_pointer {};
template<> struct fex_gen_config<glXGetProcAddressARB>: fexgen::custom_guest_entrypoint, fexgen::no_host_impl, fexgen::returns_guest_pointer {};
}

namespace egl {
template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable {
    const char* load_host_endpoint_via = "symbolFromEglGetProcAddr";
};

// EGL
template<> struct fex_gen_config<eglGetDisplay>: fexgen::custom_guest_entrypoint, fexgen::custom_host_impl {};

template<> struct fex_gen_config<eglChooseConfig>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglDestroyContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglDestroySurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglInitialize>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglMakeCurrent>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglQuerySurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglSurfaceAttrib>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglSwapBuffers>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglTerminate>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreateContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreateWindowSurface>: fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<eglBindAPI>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglGetError> {};
template<> struct fex_gen_config<eglGetCurrentContext> {};
template<> struct fex_gen_config<eglGetCurrentDisplay> {};
template<> struct fex_gen_config<eglGetCurrentSurface> {};

// new ones
template<> struct fex_gen_config<eglBindTexImage>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglClientWaitSync>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCopyBuffers>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreateImage>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreatePbufferFromClientBuffer>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreatePbufferSurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreatePixmapSurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreatePlatformPixmapSurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreatePlatformWindowSurface>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglCreateSync>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglDestroyImage>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglDestroySync>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglGetConfigAttrib>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglGetConfigs>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglGetPlatformDisplay>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglGetSyncAttrib>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglQueryContext>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglQueryString>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglReleaseTexImage>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglSwapInterval>: fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<eglWaitSync>: fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<eglQueryAPI> {};
template<> struct fex_gen_config<eglReleaseThread> {};
template<> struct fex_gen_config<eglWaitClient> {};
template<> struct fex_gen_config<eglWaitGL> {};
template<> struct fex_gen_config<eglWaitNative> {};

template<> struct fex_gen_config<eglGetProcAddress>: fexgen::custom_guest_entrypoint, fexgen::no_host_impl, fexgen::returns_guest_pointer {};
}


// Symbols looked up through glXGetProcAddr
namespace gl {

template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable {
    const char* load_host_endpoint_via = "symbolFromGlXGetProcAddr";
};

// gl, glext, gles1, gles1ext, gles2, gles3
#include "libFGL_interface_gl.inl"

} // namespace internal
