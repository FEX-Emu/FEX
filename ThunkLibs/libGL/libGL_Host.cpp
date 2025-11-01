/*
$info$
tags: thunklibs|GL
desc: Uses glXGetProcAddress instead of dlsym
$end_info$
*/

#include <cstdio>
#include <cstdlib>
#include <string_view>

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <xcb/xcb.h>

#include "common/Host.h"
#include "common/X11Manager.h"

template<>
struct host_layout<_XDisplay*> {
  _XDisplay* data;
  _XDisplay* guest_display;

  host_layout(guest_layout<_XDisplay*>&);

  ~host_layout();
};

static X11Manager x11_manager;

static void* (*GuestMalloc)(guest_size_t) = nullptr;

host_layout<_XDisplay*>::host_layout(guest_layout<_XDisplay*>& guest)
  : guest_display(guest.force_get_host_pointer()) {
  data = x11_manager.GuestToHostDisplay(guest_display);
}

host_layout<_XDisplay*>::~host_layout() {
  // Flush host-side event queue to make effects of the guest-side connection visible
  x11_manager.HostXFlush(data);
}

// Functions returning _XDisplay* should be handled explicitly via ptr_passthrough
guest_layout<_XDisplay*> to_guest(host_layout<_XDisplay*>) = delete;

static void fexfn_impl_libGL_GL_SetGuestMalloc(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &GuestMalloc);
}

static void fexfn_impl_libGL_GL_SetGuestXGetVisualInfo(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXGetVisualInfo);
}

static void fexfn_impl_libGL_GL_SetGuestXSync(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXSync);
}

static void fexfn_impl_libGL_GL_SetGuestXDisplayString(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXDisplayString);
}

#include "thunkgen_host_libGL.inl"

auto fexfn_impl_libGL_glXGetProcAddress(const GLubyte* name) -> void (*)() {
  using VoidFn = void (*)();
  std::string_view name_sv {reinterpret_cast<const char*>(name)};
  if (name_sv == "glCompileShaderIncludeARB") {
    return (VoidFn)fexfn_impl_libGL_glCompileShaderIncludeARB;
  } else if (name_sv == "glCreateShaderProgramv") {
    return (VoidFn)fexfn_impl_libGL_glCreateShaderProgramv;
  } else if (name_sv == "glGetBufferPointerv") {
    return (VoidFn)fexfn_impl_libGL_glGetBufferPointerv;
  } else if (name_sv == "glGetBufferPointervARB") {
    return (VoidFn)fexfn_impl_libGL_glGetBufferPointervARB;
  } else if (name_sv == "glGetNamedBufferPointerv") {
    return (VoidFn)fexfn_impl_libGL_glGetNamedBufferPointerv;
  } else if (name_sv == "glGetNamedBufferPointervEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetNamedBufferPointervEXT;
  } else if (name_sv == "glGetPointerv") {
    return (VoidFn)fexfn_impl_libGL_glGetPointerv;
  } else if (name_sv == "glGetPointervEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetPointervEXT;
  } else if (name_sv == "glGetPointeri_vEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetPointeri_vEXT;
  } else if (name_sv == "glGetPointerIndexedvEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetPointerIndexedvEXT;
  } else if (name_sv == "glGetVariantPointervEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetVariantPointervEXT;
  } else if (name_sv == "glGetVertexAttribPointervARB") {
    return (VoidFn)fexfn_impl_libGL_glGetVertexAttribPointervARB;
  } else if (name_sv == "glGetVertexAttribPointerv") {
    return (VoidFn)fexfn_impl_libGL_glGetVertexAttribPointerv;
  } else if (name_sv == "glGetVertexAttribPointervNV") {
    return (VoidFn)fexfn_impl_libGL_glGetVertexAttribPointervNV;
  } else if (name_sv == "glGetVertexArrayPointeri_vEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetVertexArrayPointeri_vEXT;
  } else if (name_sv == "glGetVertexArrayPointervEXT") {
    return (VoidFn)fexfn_impl_libGL_glGetVertexArrayPointervEXT;
  } else if (name_sv == "glShaderSource") {
    return (VoidFn)fexfn_impl_libGL_glShaderSource;
  } else if (name_sv == "glShaderSourceARB") {
    return (VoidFn)fexfn_impl_libGL_glShaderSourceARB;
#ifdef IS_32BIT_THUNK
  } else if (name_sv == "glBindBuffersRange") {
    return (VoidFn)fexfn_impl_libGL_glBindBuffersRange;
  } else if (name_sv == "glBindVertexBuffers") {
    return (VoidFn)fexfn_impl_libGL_glBindVertexBuffers;
  } else if (name_sv == "glGetUniformIndices") {
    return (VoidFn)fexfn_impl_libGL_glGetUniformIndices;
  } else if (name_sv == "glVertexArrayVertexBuffers") {
    return (VoidFn)fexfn_impl_libGL_glVertexArrayVertexBuffers;
#endif
  } else if (name_sv == "glXChooseFBConfig") {
    return (VoidFn)fexfn_impl_libGL_glXChooseFBConfig;
  } else if (name_sv == "glXChooseFBConfigSGIX") {
    return (VoidFn)fexfn_impl_libGL_glXChooseFBConfigSGIX;
  } else if (name_sv == "glXGetCurrentDisplay") {
    return (VoidFn)fexfn_impl_libGL_glXGetCurrentDisplay;
  } else if (name_sv == "glXGetCurrentDisplayEXT") {
    return (VoidFn)fexfn_impl_libGL_glXGetCurrentDisplayEXT;
  } else if (name_sv == "glXGetFBConfigs") {
    return (VoidFn)fexfn_impl_libGL_glXGetFBConfigs;
  } else if (name_sv == "glXGetFBConfigFromVisualSGIX") {
    return (VoidFn)fexfn_impl_libGL_glXGetFBConfigFromVisualSGIX;
  } else if (name_sv == "glXGetVisualFromFBConfigSGIX") {
    return (VoidFn)fexfn_impl_libGL_glXGetVisualFromFBConfigSGIX;
  } else if (name_sv == "glXChooseVisual") {
    return (VoidFn)fexfn_impl_libGL_glXChooseVisual;
  } else if (name_sv == "glXCreateContext") {
    return (VoidFn)fexfn_impl_libGL_glXCreateContext;
  } else if (name_sv == "glXCreateGLXPixmap") {
    return (VoidFn)fexfn_impl_libGL_glXCreateGLXPixmap;
  } else if (name_sv == "glXCreateGLXPixmapMESA") {
    return (VoidFn)fexfn_impl_libGL_glXCreateGLXPixmapMESA;
  } else if (name_sv == "glXGetConfig") {
    return (VoidFn)fexfn_impl_libGL_glXGetConfig;
  } else if (name_sv == "glXGetVisualFromFBConfig") {
    return (VoidFn)fexfn_impl_libGL_glXGetVisualFromFBConfig;
#ifdef IS_32BIT_THUNK
  } else if (name_sv == "glXGetSelectedEvent") {
    return (VoidFn)fexfn_impl_libGL_glXGetSelectedEvent;
  } else if (name_sv == "glXGetSelectedEventSGIX") {
    return (VoidFn)fexfn_impl_libGL_glXGetSelectedEventSGIX;
#endif
  }
  return (VoidFn)glXGetProcAddress((const GLubyte*)name);
}

// TODO: unsigned int *glXEnumerateVideoDevicesNV (Display *dpy, int screen, int *nelements);


void fexfn_impl_libGL_glCompileShaderIncludeARB(GLuint a_0, GLsizei Count, guest_layout<const GLchar* const*> a_2, const GLint* a_3) {
#ifndef IS_32BIT_THUNK
  auto sources = a_2.force_get_host_pointer();
#else
  auto sources = (const char**)alloca(Count * sizeof(const char*));
  for (GLsizei i = 0; i < Count; ++i) {
    sources[i] = host_layout<const char* const> {a_2.get_pointer()[i]}.data;
  }
#endif
  return fexldr_ptr_libGL_glCompileShaderIncludeARB(a_0, Count, sources, a_3);
}

GLuint fexfn_impl_libGL_glCreateShaderProgramv(GLuint a_0, GLsizei count, guest_layout<const GLchar* const*> a_2) {
#ifndef IS_32BIT_THUNK
  auto sources = a_2.force_get_host_pointer();
#else
  auto sources = (const char**)alloca(count * sizeof(const char*));
  for (GLsizei i = 0; i < count; ++i) {
    sources[i] = host_layout<const char* const> {a_2.get_pointer()[i]}.data;
  }
#endif
  return fexldr_ptr_libGL_glCreateShaderProgramv(a_0, count, sources);
}

void fexfn_impl_libGL_glGetBufferPointerv(GLenum a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetBufferPointerv(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetBufferPointervARB(GLenum a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetBufferPointervARB(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetNamedBufferPointerv(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetNamedBufferPointerv(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetNamedBufferPointervEXT(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetNamedBufferPointervEXT(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetPointerv(GLenum a_0, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetPointerv(a_0, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetPointervEXT(GLenum a_0, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetPointervEXT(a_0, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetPointeri_vEXT(GLenum a_0, GLuint a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetPointeri_vEXT(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetPointerIndexedvEXT(GLenum a_0, GLuint a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetPointerIndexedvEXT(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVariantPointervEXT(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVariantPointervEXT(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVertexAttribPointervARB(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVertexAttribPointervARB(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVertexAttribPointerv(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVertexAttribPointerv(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVertexAttribPointervNV(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVertexAttribPointervNV(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVertexArrayPointeri_vEXT(GLuint a_0, GLuint a_1, GLenum a_2, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVertexArrayPointeri_vEXT(a_0, a_1, a_2, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glGetVertexArrayPointervEXT(GLuint a_0, GLenum a_1, guest_layout<void**> GuestOut) {
  void* HostOut;
  fexldr_ptr_libGL_glGetVertexArrayPointervEXT(a_0, a_1, &HostOut);
  *GuestOut.get_pointer() = to_guest(to_host_layout(HostOut));
}

void fexfn_impl_libGL_glShaderSource(GLuint a_0, GLsizei count, guest_layout<const GLchar* const*> a_2, const GLint* a_3) {
#ifndef IS_32BIT_THUNK
  auto sources = a_2.force_get_host_pointer();
#else
  auto sources = (const char**)alloca(count * sizeof(const char*));
  for (GLsizei i = 0; i < count; ++i) {
    sources[i] = host_layout<const char* const> {a_2.get_pointer()[i]}.data;
  }
#endif
  return fexldr_ptr_libGL_glShaderSource(a_0, count, sources, a_3);
}

void fexfn_impl_libGL_glShaderSourceARB(GLuint a_0, GLsizei count, guest_layout<const GLcharARB**> a_2, const GLint* a_3) {
#ifndef IS_32BIT_THUNK
  auto sources = a_2.force_get_host_pointer();
#else
  auto sources = (const char**)alloca(count * sizeof(const char*));
  for (GLsizei i = 0; i < count; ++i) {
    sources[i] = a_2.get_pointer()[i].force_get_host_pointer();
  }
#endif
  return fexldr_ptr_libGL_glShaderSourceARB(a_0, count, sources, a_3);
}

// Relocate data to guest heap so it can be called with XFree.
// The memory at the given host location will be de-allocated.
template<typename T>
guest_layout<T*> RelocateArrayToGuestHeap(T* Data, int NumItems) {
  if (!Data) {
    return guest_layout<T*> {.data = 0};
  }

  guest_layout<T*> GuestData;
  GuestData.data = reinterpret_cast<uintptr_t>(GuestMalloc(sizeof(guest_layout<T>) * NumItems));
  for (int Index = 0; Index < NumItems; ++Index) {
    GuestData.get_pointer()[Index] = to_guest(to_host_layout(Data[Index]));
  }
  x11_manager.HostXFree(Data);
  return GuestData;
}

// Maps to a host-side XVisualInfo, which must be XFree'ed by the caller.
static XVisualInfo* LookupHostVisualInfo(Display* HostDisplay, guest_layout<XVisualInfo*> GuestInfo) {
  if (!GuestInfo.data) {
    return nullptr;
  }

  int num_matches;
  auto HostInfo = host_layout<XVisualInfo> {*GuestInfo.get_pointer()}.data;
  auto ret = x11_manager.HostXGetVisualInfo(HostDisplay, uint64_t {VisualScreenMask | VisualIDMask}, &HostInfo, &num_matches);
  if (num_matches != 1) {
    fprintf(stderr, "ERROR: Did not find unique host XVisualInfo\n");
    std::abort();
  }
  return ret;
}

// Maps to a guest-side XVisualInfo and destroys the host argument.
static guest_layout<XVisualInfo*> MapToGuestVisualInfo(Display* HostDisplay, XVisualInfo* HostInfo) {
  if (!HostInfo) {
    return guest_layout<XVisualInfo*> {.data = 0};
  }

  auto guest_display = x11_manager.HostToGuestDisplay(HostDisplay);
#ifndef IS_32BIT_THUNK
  int num_matches;
  auto GuestInfo = to_guest(to_host_layout(*HostInfo));
#else
  GuestStackBumpAllocator GuestStack;
  auto& num_matches = *GuestStack.New<int>();
  auto& GuestInfo = *GuestStack.New<guest_layout<XVisualInfo>>(to_guest(to_host_layout(*HostInfo)));
#endif
  auto ret = x11_manager.GuestXGetVisualInfo(guest_display.get_pointer(), VisualScreenMask | VisualIDMask, &GuestInfo, &num_matches);

  if (num_matches != 1) {
    fprintf(stderr, "ERROR: Did not find unique guest XVisualInfo\n");
    std::abort();
  }

  // We effectively relocated the VisualInfo, so free the original one now
  x11_manager.HostXFree(HostInfo);
  guest_layout<XVisualInfo*> GuestRet;
  GuestRet.data = reinterpret_cast<uintptr_t>(ret);
  return GuestRet;
}

guest_layout<GLXFBConfig*> fexfn_impl_libGL_glXChooseFBConfig(Display* Display, int Screen, const int* Attributes, int* NumItems) {
  auto ret = fexldr_ptr_libGL_glXChooseFBConfig(Display, Screen, Attributes, NumItems);
  return RelocateArrayToGuestHeap(ret, *NumItems);
}

guest_layout<GLXFBConfigSGIX*> fexfn_impl_libGL_glXChooseFBConfigSGIX(Display* Display, int Screen, int* Attributes, int* NumItems) {
  auto ret = fexldr_ptr_libGL_glXChooseFBConfigSGIX(Display, Screen, Attributes, NumItems);
  return RelocateArrayToGuestHeap(ret, *NumItems);
}

guest_layout<_XDisplay*> fexfn_impl_libGL_glXGetCurrentDisplay() {
  auto ret = fexldr_ptr_libGL_glXGetCurrentDisplay();
  return x11_manager.HostToGuestDisplay(ret);
}

guest_layout<_XDisplay*> fexfn_impl_libGL_glXGetCurrentDisplayEXT() {
  auto ret = fexldr_ptr_libGL_glXGetCurrentDisplayEXT();
  return x11_manager.HostToGuestDisplay(ret);
}

guest_layout<GLXFBConfig*> fexfn_impl_libGL_glXGetFBConfigs(Display* Display, int Screen, int* NumItems) {
  auto ret = fexldr_ptr_libGL_glXGetFBConfigs(Display, Screen, NumItems);
  return RelocateArrayToGuestHeap(ret, *NumItems);
}

GLXFBConfigSGIX fexfn_impl_libGL_glXGetFBConfigFromVisualSGIX(Display* Display, guest_layout<XVisualInfo*> Info) {
  auto HostInfo = LookupHostVisualInfo(Display, Info);
  auto ret = fexldr_ptr_libGL_glXGetFBConfigFromVisualSGIX(Display, HostInfo);
  x11_manager.HostXFree(HostInfo);
  return ret;
}

guest_layout<XVisualInfo*> fexfn_impl_libGL_glXGetVisualFromFBConfigSGIX(Display* Display, GLXFBConfigSGIX Config) {
  return MapToGuestVisualInfo(Display, fexldr_ptr_libGL_glXGetVisualFromFBConfigSGIX(Display, Config));
}

guest_layout<XVisualInfo*> fexfn_impl_libGL_glXChooseVisual(Display* Display, int Screen, int* Attributes) {
  return MapToGuestVisualInfo(Display, fexldr_ptr_libGL_glXChooseVisual(Display, Screen, Attributes));
}

GLXContext fexfn_impl_libGL_glXCreateContext(Display* Display, guest_layout<XVisualInfo*> Info, GLXContext ShareList, Bool Direct) {
  auto HostInfo = LookupHostVisualInfo(Display, Info);
  auto ret = fexldr_ptr_libGL_glXCreateContext(Display, HostInfo, ShareList, Direct);
  x11_manager.HostXFree(HostInfo);
  return ret;
}

GLXPixmap fexfn_impl_libGL_glXCreateGLXPixmap(Display* Display, guest_layout<XVisualInfo*> Info, Pixmap Pixmap) {
  auto HostInfo = LookupHostVisualInfo(Display, Info);
  auto ret = fexldr_ptr_libGL_glXCreateGLXPixmap(Display, HostInfo, Pixmap);
  x11_manager.HostXFree(HostInfo);
  return ret;
}

GLXPixmap fexfn_impl_libGL_glXCreateGLXPixmapMESA(Display* Display, guest_layout<XVisualInfo*> Info, Pixmap Pixmap, Colormap Colormap) {
  auto HostInfo = LookupHostVisualInfo(Display, Info);
  auto ret = fexldr_ptr_libGL_glXCreateGLXPixmapMESA(Display, HostInfo, Pixmap, Colormap);
  x11_manager.HostXFree(HostInfo);
  return ret;
}

int fexfn_impl_libGL_glXGetConfig(Display* Display, guest_layout<XVisualInfo*> Info, int Attribute, int* Value) {
  auto HostInfo = LookupHostVisualInfo(Display, Info);
  auto ret = fexldr_ptr_libGL_glXGetConfig(Display, HostInfo, Attribute, Value);
  x11_manager.HostXFree(HostInfo);
  return ret;
}

guest_layout<XVisualInfo*> fexfn_impl_libGL_glXGetVisualFromFBConfig(Display* Display, GLXFBConfig Config) {
  return MapToGuestVisualInfo(Display, fexldr_ptr_libGL_glXGetVisualFromFBConfig(Display, Config));
}

#ifdef IS_32BIT_THUNK
void fexfn_impl_libGL_glBindBuffersRange(GLenum a_0, GLuint a_1, GLsizei Count, const GLuint* a_3, guest_layout<const int*> Offsets,
                                         guest_layout<const int*> Sizes) {
  auto HostOffsets = (GLintptr*)alloca(Count * sizeof(GLintptr));
  auto HostSizes = (GLsizeiptr*)alloca(Count * sizeof(GLsizeiptr));
  for (int i = 0; i < Count; ++i) {
    HostOffsets[i] = Offsets.get_pointer()[i].data;
    HostSizes[i] = Sizes.get_pointer()[i].data;
  }
  return fexldr_ptr_libGL_glBindBuffersRange(a_0, a_1, Count, a_3, HostOffsets, HostSizes);
}

void fexfn_impl_libGL_glBindVertexBuffers(GLuint a_0, GLsizei count, const GLuint* a_2, guest_layout<const int*> Offsets, const GLsizei* a_4) {
  auto HostOffsets = (GLintptr*)alloca(count * sizeof(GLintptr));
  for (int i = 0; i < count; ++i) {
    HostOffsets[i] = Offsets.get_pointer()[i].data;
  }
  fexldr_ptr_libGL_glBindVertexBuffers(a_0, count, a_2, HostOffsets, a_4);
}

void fexfn_impl_libGL_glGetUniformIndices(GLuint a_0, GLsizei Count, guest_layout<const GLchar* const*> Names, GLuint* a_3) {
  auto HostNames = (const GLchar**)alloca(Count * sizeof(GLintptr));
  for (int i = 0; i < Count; ++i) {
    HostNames[i] = host_layout<const char* const> {Names.get_pointer()[i]}.data;
  }
  fexldr_ptr_libGL_glGetUniformIndices(a_0, Count, HostNames, a_3);
}

void fexfn_impl_libGL_glVertexArrayVertexBuffers(GLuint a_0, GLuint a_1, GLsizei count, const GLuint* a_3, guest_layout<const int*> Offsets,
                                                 const GLsizei* a_5) {
  auto HostOffsets = (GLintptr*)alloca(count * sizeof(GLintptr));
  for (int i = 0; i < count; ++i) {
    HostOffsets[i] = Offsets.get_pointer()[i].data;
  }
  fexldr_ptr_libGL_glVertexArrayVertexBuffers(a_0, a_1, count, a_3, HostOffsets, a_5);
}

void fexfn_impl_libGL_glXGetSelectedEvent(Display* Display, GLXDrawable Drawable, guest_layout<uint32_t*> Mask) {
  unsigned long HostMask;
  fexldr_ptr_libGL_glXGetSelectedEvent(Display, Drawable, &HostMask);
  *Mask.get_pointer() = HostMask;
}
void fexfn_impl_libGL_glXGetSelectedEventSGIX(Display* Display, GLXDrawable Drawable, guest_layout<uint32_t*> Mask) {
  unsigned long HostMask;
  fexldr_ptr_libGL_glXGetSelectedEventSGIX(Display, Drawable, &HostMask);
  *Mask.get_pointer() = HostMask;
}
#endif

EXPORTS(libGL)
