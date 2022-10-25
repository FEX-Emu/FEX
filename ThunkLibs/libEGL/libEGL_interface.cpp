#include <common/GeneratorInterface.h>

#include <EGL/egl.h>

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};

template<> struct fex_gen_config<eglBindAPI> {};
template<> struct fex_gen_config<eglChooseConfig> {};
template<> struct fex_gen_config<eglDestroyContext> {};
template<> struct fex_gen_config<eglDestroySurface> {};
template<> struct fex_gen_config<eglInitialize> {};
template<> struct fex_gen_config<eglMakeCurrent> {};
template<> struct fex_gen_config<eglQuerySurface> {};
template<> struct fex_gen_config<eglSurfaceAttrib> {};
template<> struct fex_gen_config<eglSwapBuffers> {};
template<> struct fex_gen_config<eglTerminate> {};
template<> struct fex_gen_config<eglGetError> {};
template<> struct fex_gen_config<eglCreateContext> {};
template<> struct fex_gen_config<eglCreateWindowSurface> {};
template<> struct fex_gen_config<eglGetCurrentContext> {};
template<> struct fex_gen_config<eglGetCurrentDisplay> {};
template<> struct fex_gen_config<eglGetCurrentSurface> {};
template<> struct fex_gen_config<eglGetDisplay> {};
